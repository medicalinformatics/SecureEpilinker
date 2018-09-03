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
  ServerHandler::ServerHandler(size_t worker_threads) {
  while (worker_threads-->0){
    m_worker_threads.emplace_back([this](){execute_job_queue();});
  }
  }


  ServerHandler& ServerHandler::get() {
    static ServerHandler singleton{2}; //FIXME(TK) Magic Number
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
  m_logger->debug("Creating client on port {}, Remote host: {}", aby_config.port, aby_config.remote_host);
  m_aby_clients.emplace(id, make_shared<SecureEpilinker>(aby_config,circuit_config));
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
      move(remote_address.ip),
      remote_address.port, aby_info.aby_threads};
  m_logger->debug("Creating server on port {}, Remote host: {}\n", aby_config.port, aby_config.remote_host);
  m_server.emplace(id, make_shared<LocalServer>(id, aby_config, circuit_config));
  get_local_server(id)->connect_server();
}

void ServerHandler::add_linkage_job(const RemoteId& remote_id, std::shared_ptr<LinkageJob>&& job){
  const auto& config_handler{ConfigurationHandler::cget()};
  const auto job_id{job->get_id()};
  if(config_handler.get_remote_config(remote_id)->get_mutual_initialization_status()) {
  m_job_remote_mapping.emplace(job->get_id(), remote_id);
  m_client_jobs[remote_id].emplace(job->get_id(),move(job));
  } else {
    m_logger->error("Can not create linkage job {}: Connection to remote Secure EpiLinker is not properly initialized", job_id);
  }
}

void ServerHandler::run_job(const RemoteId& remote_id, shared_ptr<LinkageJob>& job){
  const auto& config_handler{ConfigurationHandler::cget()};
  const auto& job_id{job->get_id()};
  if(!config_handler.get_remote_config(remote_id)->get_matching_mode()){
    m_client_jobs.at(remote_id).at(job_id)->run_linkage_job();
  } else {
#ifdef SEL_MATCHING_MODE
    m_client_jobs.at(remote_id).at(job_id)->run_matching_job();
#endif
  }
}

void ServerHandler::execute_job_queue() {
  // Loops forever. Watches queue of jobs. If a job of one remote is found (and
  // not already finished, running or faulty it starts that job. After that the
  // next remote host is chosen. This should guarantee a fair distribution of
  // jobs, i.e. round robbin w.r.t. remote hosts.
  for(;;){
    size_t counter{0};
    while (counter < m_client_jobs.size()){
      lock_guard<mutex> lock{m_job_queue_mutex};
      RemoteId remote_id{};
      shared_ptr<LinkageJob> job{nullptr};
      bool matching_mode{false};
      for (auto& jobp : std::next(m_client_jobs.begin(),counter)->second){
        if(jobp.second->get_status() == JobStatus::QUEUED) {
          remote_id = std::next(m_client_jobs.begin(),counter)->first;
          job = jobp.second;
          jobp.second->set_status(JobStatus::RUNNING);
          matching_mode = ConfigurationHandler::cget().get_remote_config(remote_id)->get_matching_mode();
          break;
        }
      }
      ++counter;
      if(job){
        if(!matching_mode)
          job->run_linkage_job();
        else
          job->run_matching_job();
      }
    }
  }
}

shared_ptr<const LinkageJob> ServerHandler::get_linkage_job(const JobId& j_id) const {
  return m_client_jobs.at(m_job_remote_mapping.at(j_id)).at(j_id);
}

string ServerHandler::get_job_status(const JobId& j_id) const {
  if (j_id == "list"){ // Generate job status listing
    nlohmann::json result;
    for(const auto& remote_queue : m_client_jobs) {
      for(const auto& job : remote_queue.second) {
        result[job.first] = js_enum_to_string(job.second->get_status());
      }
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
                               std::shared_ptr<const ServerData> data) {
  const auto& config_handler{ConfigurationHandler::cget()};
  auto remote_config{config_handler.get_remote_config(remote_id)};
  auto local_config{config_handler.get_local_config()};
  if (remote_config->get_mutual_initialization_status()) {
    const auto result{
        get_local_server(remote_id)->launch_comparison(move(data))};
    m_logger->info("Server Result\n{}", result);
    if(!remote_config->get_matching_mode()){
      const auto ids{get_local_server(remote_id)->get_ids()};
      string id_string;
      for (size_t i = 0; i != ids.size(); ++i) {
        id_string += "Index: " + to_string(i) + " ID: " + ids.at(i) + '\n';
      }
      m_logger->info("IDs:\n{}", id_string);
      auto linkage_service{remote_config->get_linkage_service()};
      string url{linkage_service->url + "/linkageResult/" +
                 local_config->get_local_id() + '/' + remote_id};
      m_logger->debug("Sending server result to Linkage Service URL {}", url);
      try {
      auto response{send_result_to_linkageservice(result, make_optional(ids), "server",
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
