/**
\file    configurationhandler.cpp
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
\brief Class to handle Local and Remote Configurations
*/

#include "configurationhandler.h"
#include <memory>
#include <mutex>
#include "connectionhandler.h"
#include "localconfiguration.h"
#include "remoteconfiguration.h"
#include "resttypes.h"
#include "logger.h"

using namespace std;
namespace sel {

void ConfigurationHandler::set_remote_config(
    shared_ptr<RemoteConfiguration>&& remote) {
  lock_guard<mutex> lock(m_remote_mutex);
  m_remote_configs[remote->get_id()] = remote;
}

void ConfigurationHandler::set_algorithm_config(
    std::shared_ptr<AlgorithmConfig>&& algo) {
  lock_guard<mutex> lock(m_algo_mutex);
  m_algo_config = algo;
}

void ConfigurationHandler::set_local_config(
    shared_ptr<LocalConfiguration>&& local) {
  lock_guard<mutex> lock(m_local_mutex);
  m_local_config = local;
}

shared_ptr<const LocalConfiguration> ConfigurationHandler::get_local_config()
    const {
  lock_guard<mutex> lock(m_local_mutex);
  return m_local_config;
}

shared_ptr<const AlgorithmConfig> ConfigurationHandler::get_algorithm_config()
    const {
  lock_guard<mutex> lock(m_algo_mutex);
  return m_algo_config;
}

shared_ptr<RemoteConfiguration> ConfigurationHandler::get_remote_config(
    const RemoteId& remote_id) const {
  lock_guard<mutex> lock(m_remote_mutex);
  try{
  return m_remote_configs.at(remote_id);
  } catch  (const out_of_range& e){
    auto logger{get_default_logger()};
    logger->error("Error in get_remote_config. Remote Id {} is unknown: {}", 
                                                            remote_id, e.what());
  }
}

size_t ConfigurationHandler::get_remote_count() const {
  lock_guard<mutex> lock(m_remote_mutex);
  return m_remote_configs.size();
}

void ConfigurationHandler::set_server_config(ServerConfig&& server_config){
  m_server_config = move(server_config);
}

ServerConfig ConfigurationHandler::get_server_config() const {
  return m_server_config;
}

EpilinkConfig make_epilink_config(shared_ptr<const LocalConfiguration> local_config,
                                    shared_ptr<const AlgorithmConfig> algo_config){
return {local_config->get_fields(),
        local_config->get_exchange_groups(),
        algo_config->threshold_match,
        algo_config->threshold_non_match };
}
}  // namespace sel


