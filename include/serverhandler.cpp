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
#include <tuple>
using namespace std;
namespace sel {
ServerHandler::ServerHandler(
    std::shared_ptr<ConfigurationHandler> config_handler,
    std::shared_ptr<DataHandler> data_handler)
    : m_config_handler{move(config_handler)},
      m_data_handler{move(data_handler)} {}

void ServerHandler::insert_client(RemoteId id) {
  auto local_config{m_config_handler->get_local_config()};
  auto epilink_config{get_epilink_config(
      local_config, m_config_handler->get_algorithm_config())};
  auto aby_info{local_config->get_aby_info()};
  auto remote_config{m_config_handler->get_remote_config(id)};
  SecureEpilinker::ABYConfig aby_config{
      CLIENT, aby_info.boolean_sharing, remote_config->get_remote_host(),
      remote_config->get_aby_port(), aby_info.aby_threads};
  fmt::print("Creating client on port {}, Remote host: {}\n", aby_config.port, aby_config.remote_host);
  m_aby_clients.emplace(id, make_shared<SecureEpilinker>(aby_config,epilink_config));
}

void ServerHandler::insert_server(ClientId id, RemoteAddress remote_address) {
  auto local_config{m_config_handler->get_local_config()};
  auto epilink_config{get_epilink_config(
      local_config, m_config_handler->get_algorithm_config())};
  auto aby_info{local_config->get_aby_info()};
  SecureEpilinker::ABYConfig aby_config{
      SERVER, aby_info.boolean_sharing, move(remote_address.ip),
      remote_address.port, aby_info.aby_threads};
  fmt::print("Creating server on port {}, Remote host: {}\n", aby_config.port, aby_config.remote_host);
  m_server.emplace(id, make_shared<LocalServer>(id, aby_config, epilink_config, m_data_handler, m_config_handler));
}

void ServerHandler::add_linkage_job(const RemoteId& remote_id, std::shared_ptr<LinkageJob>&& job){
  const auto job_id{job->get_id()};
  m_job_remote_mapping.emplace(job->get_id(), remote_id);
  m_client_jobs[remote_id].emplace(job->get_id(),move(job));
  m_client_jobs.at(remote_id).at(job_id)->run_job();
}

shared_ptr<const LinkageJob> ServerHandler::get_linkage_job(const JobId& j_id) const {
  return m_client_jobs.at(m_job_remote_mapping.at(j_id)).at(j_id);
}

uint16_t ServerHandler::get_server_port(const ClientId& id) const {
  return m_server.at(id)->get_port();
}
shared_ptr<SecureEpilinker> ServerHandler::get_epilink_client(const RemoteId& remote_id){
  return m_aby_clients.at(remote_id);
}

std::shared_ptr<LocalServer> ServerHandler::get_local_server(const ClientId& client_id) const {
  return m_server.at(client_id);
}
}  // namespace sel
