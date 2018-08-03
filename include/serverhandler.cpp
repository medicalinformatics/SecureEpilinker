/**
\file    serverhandler.cpp
\author  Tobias Kussel <kussel@cbs.tu-darmstadt.de>
\copyright SEL - Secure EpiLinker
    Copyright (C) 2018 Computational Biology & Simulation Group TU-Darmstadt
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Affero General Public License for more details.
    You should have received a copy of the GNU Affero General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
\brief Holds and handles ABY server and clients
*/

#include "serverhandler.h"
#include <string>
#include "configurationhandler.h"
#include "localconfiguration.h"
#include "remoteconfiguration.h"
#include "secure_epilinker.h"
#include "connectionhandler.h"
#include "epilink_input.h"
#include "seltypes.h"
#include "resttypes.h"
#include <tuple>
#include "logger.h"

using namespace std;
namespace sel {
ServerHandler::ServerHandler(
    std::shared_ptr<ConfigurationHandler> config_handler,
    std::shared_ptr<DataHandler> data_handler)
    : m_config_handler{move(config_handler)},
      m_data_handler{move(data_handler)}, m_logger{get_default_logger()} {}

void ServerHandler::insert_client(RemoteId id) {
  auto local_config{m_config_handler->get_local_config()};
  auto remote_config{m_config_handler->get_remote_config(id)};
  auto epilink_config{make_epilink_config(
      local_config, m_config_handler->get_algorithm_config(), remote_config->get_matching_mode())};
  if(remote_config->get_matching_mode()){
    m_logger->warn("Client created with matching mode enanabled!");
  }
  //epilink_config.set_precisions(6,11);
  m_logger->debug("Client Precision: Dice {},\tWeight {}", epilink_config.dice_prec, epilink_config.weight_prec);
  auto aby_info{m_config_handler->get_server_config()};
  SecureEpilinker::ABYConfig aby_config{
      CLIENT, aby_info.boolean_sharing, remote_config->get_remote_host(),
      remote_config->get_aby_port(), aby_info.aby_threads};
  m_logger->debug("Creating client on port {}, Remote host: {}", aby_config.port, aby_config.remote_host);
  m_aby_clients.emplace(id, make_shared<SecureEpilinker>(aby_config,epilink_config));
}

void ServerHandler::insert_server(RemoteId id, RemoteAddress remote_address) {
  auto local_config{m_config_handler->get_local_config()};
  auto remote_config{m_config_handler->get_remote_config(id)};
  auto epilink_config{make_epilink_config(
      local_config, m_config_handler->get_algorithm_config(), remote_config->get_matching_mode())};
  if(remote_config->get_matching_mode()){
    m_logger->warn("Server created with matching mode enanabled!");
  }
  //epilink_config.set_precisions(6,11);
  m_logger->debug("Server Precision: Dice {},\tWeight {}", epilink_config.dice_prec, epilink_config.weight_prec);
  auto aby_info{m_config_handler->get_server_config()};
  SecureEpilinker::ABYConfig aby_config{
      SERVER, aby_info.boolean_sharing, move(remote_address.ip),
      remote_address.port, aby_info.aby_threads};
  m_logger->debug("Creating server on port {}, Remote host: {}\n", aby_config.port, aby_config.remote_host);
  m_server.emplace(id, make_shared<LocalServer>(id, aby_config, epilink_config, m_data_handler, m_config_handler));
}

void ServerHandler::add_linkage_job(const RemoteId& remote_id, std::shared_ptr<LinkageJob>&& job){
  const auto job_id{job->get_id()};
  if(m_config_handler->get_remote_config(remote_id)->get_mutual_initialization_status()) {
  m_job_remote_mapping.emplace(job->get_id(), remote_id);
  m_client_jobs[remote_id].emplace(job->get_id(),move(job));
  try{
  m_client_jobs.at(remote_id).at(job_id)->run_linkage_job();
  } catch (const exception& e) {
    m_logger->error("Error in add_linkage_job: {}", e.what());
  }
  } else {
    m_logger->error("Can not create linkage job {}: Connection to remote Secure EpiLinker is not properly initialized", job_id);
  }
}

shared_ptr<const LinkageJob> ServerHandler::get_linkage_job(const JobId& j_id) const {
  try{
  return m_client_jobs.at(m_job_remote_mapping.at(j_id)).at(j_id);
  } catch (const exception& e) {
    m_logger->error("Error in get_linkage_job: {}", e.what());
  }
  // this return is to silence the compiler warning and will never be executed
  return nullptr;
}

Port ServerHandler::get_server_port(const RemoteId& id) const {
  try{
  return m_server.at(id)->get_port();
  } catch (const exception& e) {
    m_logger->error("Error in get_server_port: {}", e.what());
  }
  // this return is to silence the compiler warning and will never be executed
  return 0u;
}
shared_ptr<SecureEpilinker> ServerHandler::get_epilink_client(const RemoteId& remote_id){
  try{
  return m_aby_clients.at(remote_id);
  } catch (const exception& e) {
    m_logger->error("Error in get_epilink_client: {}", e.what());
  }
  // this return is to silence the compiler warning and will never be executed
  return nullptr;
}

std::shared_ptr<LocalServer> ServerHandler::get_local_server(const RemoteId& remote_id) const {
  try{
  return m_server.at(remote_id);
  } catch (const exception& e) {
    m_logger->error("Error in get_local_server: {}", e.what());
  }
  // this return is to silence the compiler warning and will never be executed
  return nullptr;
}

void ServerHandler::run_server(RemoteId remote_id, std::shared_ptr<const ServerData> data) {
  if(m_config_handler->get_remote_config(remote_id)->get_mutual_initialization_status()){
  const auto result{get_local_server(remote_id)->launch_comparison(move(data))};
  m_logger->info("Server Result\n{}",result);
  const auto ids{get_local_server(remote_id)->get_ids()};
  string id_string;
  for (size_t i = 0; i!= ids.size(); ++i){
    id_string += "Index: "+ to_string(i) + " IDs: ";
    for (const auto& id : ids[i]){
      id_string += "\"" + id.first + ":" + id.second + "\"";
    }
    id_string += "\n";
  }
  m_logger->info("IDs:\n{}", id_string);
  // TODO(TK): Send result and ids to linkage server
  } else {
    m_logger->error("Can not execute linkage job server: Connection to remote Secure EpiLinker {} is not properly initialized",remote_id);
  }
}
}  // namespace sel
