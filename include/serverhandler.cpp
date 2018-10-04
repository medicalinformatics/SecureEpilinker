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
#include "apikeyconfig.hpp"
#include "configurationhandler.h"
#include "localconfiguration.h"
#include "remoteconfiguration.h"
#include "localserver.h"
#include "restutils.h"
#include "secure_epilinker.h"
#include "connectionhandler.h"
#include "epilink_input.h"
#include "seltypes.h"
#include "resttypes.h"
#include "logger.h"
#include <tuple>
#include <mutex>
#include <iterator>

using namespace std;

namespace sel {

void run_job(const shared_ptr<LinkageJob>& job) {
  assert (job->get_status() == JobStatus::QUEUED && "Only queued jobs can be run!");

  const auto& remote_id = job->get_remote_id();
  bool matching_mode = ConfigurationHandler::cget()
      .get_remote_config(remote_id)->get_matching_mode();
  if (matching_mode) {
    job->run_linkage_job();
  } else {
#ifdef SEL_MATCHING_MODE
    job->run_matching_job();
#else
    throw runtime_error("Attempt to run matching job but matching mode not compiled!");
#endif
  }
}

ServerHandler::~ServerHandler() {
  for (auto& worker_thread : m_worker_threads) {
    worker_thread.second.join();
  }
}

ServerHandler& ServerHandler::get() {
  static ServerHandler singleton;
  return singleton;
}

ServerHandler const& ServerHandler::cget(){
  return cref(get());
}

void ServerHandler::insert_client(RemoteId id) {
  const auto& config_handler{ConfigurationHandler::cget()};
  auto local_config{config_handler.get_local_config()};
  auto remote_config{config_handler.get_remote_config(id)};
  auto circuit_config{make_circuit_config(local_config, remote_config)};
  if(circuit_config.matching_mode){
    m_logger->warn("Client created with matching mode enanabled!");
  }
  auto aby_info{config_handler.get_server_config()};
  SecureEpilinker::ABYConfig aby_config{
      CLIENT, static_cast<e_sharing>(aby_info.boolean_sharing),
      remote_config->get_remote_host(),
      remote_config->get_aby_port(), aby_info.aby_threads};
  m_logger->debug("Creating client on port {}, remote host: {}", aby_config.port, aby_config.host);
  m_aby_clients.emplace(id, make_shared<SecureEpilinker>(aby_config,circuit_config));

  m_logger->debug("Creating worker thread for remote {}", id);
  m_worker_threads.emplace(id, run_job);
  connect_client(id);
}

void ServerHandler::insert_server(RemoteId id, RemoteAddress remote_address) {
  const auto& config_handler{ConfigurationHandler::cget()};
  auto local_config{config_handler.get_local_config()};
  auto remote_config{config_handler.get_remote_config(id)};
  auto circuit_config{make_circuit_config(local_config, remote_config)};
  if(circuit_config.matching_mode){
    m_logger->warn("Server created with matching mode enanabled!");
  }
  auto aby_info{config_handler.get_server_config()};
  SecureEpilinker::ABYConfig aby_config{
      SERVER, static_cast<e_sharing>(aby_info.boolean_sharing),
      ConfigurationHandler::cget().get_server_config().bind_address,
      remote_address.port, aby_info.aby_threads};
  m_logger->debug("Creating server on port {}, bound to: {}\n", aby_config.port, aby_config.host);
  m_server.emplace(id, make_shared<LocalServer>(id, aby_config, circuit_config));
  get_local_server(id)->connect_server();
}

void ServerHandler::add_linkage_job(const RemoteId& remote_id, const std::shared_ptr<LinkageJob>& job){
  const auto& config_handler = ConfigurationHandler::cget();
  const auto job_id = job->get_id();
  if(config_handler.get_remote_config(remote_id)->get_mutual_initialization_status()) {
    m_client_jobs.emplace(job_id, job);
    m_worker_threads.at(remote_id).push(job);
  } else {
    m_logger->error("Can not create linkage job {}: Connection to remote "
        "Secure EpiLinker {} is not properly initialized.", job_id, remote_id);
  }
}

shared_ptr<const LinkageJob> ServerHandler::get_linkage_job(const JobId& j_id) const {
  return m_client_jobs.at(j_id);
}

string ServerHandler::get_job_status(const JobId& j_id) const {
  if (j_id == "list"){ // Generate job status listing
    nlohmann::json result;
    for(const auto& job : m_client_jobs) {
      result[job.first] = js_enum_to_string(job.second->get_status());
    }
    return result.dump();
  } else { // get specific job status
    return js_enum_to_string(get_linkage_job(j_id)->get_status());
  }

}

Port ServerHandler::get_server_port(const RemoteId& id) const {
  return m_server.at(id)->get_port();
}

shared_ptr<SecureEpilinker> ServerHandler::get_epilink_client(const RemoteId& remote_id){
  return m_aby_clients.at(remote_id);
}

std::shared_ptr<LocalServer> ServerHandler::get_local_server(const RemoteId& remote_id) const {
  return m_server.at(remote_id);
}

void ServerHandler::run_server(RemoteId remote_id,
                               std::shared_ptr<const ServerData> data,
                               size_t num_records) {
  const auto& config_handler{ConfigurationHandler::cget()};
  auto remote_config{config_handler.get_remote_config(remote_id)};
  auto local_config{config_handler.get_local_config()};
  if (remote_config->get_mutual_initialization_status()) {
    const auto result{
        get_local_server(remote_id)->run(move(data), num_records)};
    m_logger->info("Server Result\n{}", result);
    if (!remote_config->get_matching_mode()) {
      const auto ids{get_local_server(remote_id)->get_ids()};
      string id_string;
      for (size_t i = 0; i != ids->size(); ++i) {
        id_string += "Index: " + to_string(i) + " ID: " + ids->at(i) + '\n';
      }
      m_logger->info("IDs:\n{}", id_string);
      auto linkage_service{remote_config->get_linkage_service()};
      string url{linkage_service->url + "/linkageResult/" +
                 local_config->get_local_id() + '/' + remote_id};
      m_logger->debug("Sending server result to Linkage Service URL {}", url);
      try {
      auto response{send_result_to_linkageservice(result, make_optional(*ids), "server",
                                                  local_config, remote_config)};
      m_logger->trace("Linkage Server responded with {} - {}",
                      response.return_code, response.body);
      } catch (const exception& e) {
        m_logger->error("Can not connect to linkage service: {}", e.what());
      }
    }
  } else {
    m_logger->error(
        "Can not execute linkage job server: Connection to remote Secure "
        "EpiLinker {} is not properly initialized",
        remote_id);
  }
}

void ServerHandler::connect_client(const RemoteId& remote_id) {
  m_aby_clients.at(remote_id)->connect();
}

}  // namespace sel
