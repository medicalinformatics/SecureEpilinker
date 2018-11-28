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
#include <shared_mutex>
#include "localconfiguration.h"
#include "connectionhandler.h"
#include "remoteconfiguration.h"
#include "resttypes.h"
#include "logger.h"

using namespace std;
namespace sel {

ConfigurationHandler& ConfigurationHandler::get() {
  static ConfigurationHandler singleton;
  return singleton;
}

ConfigurationHandler const& ConfigurationHandler::cget(){
  return cref(get());
}

void ConfigurationHandler::set_remote_config(
    shared_ptr<RemoteConfiguration>&& remote) {
  unique_lock<shared_mutex> lock(m_remote_mutex);
  m_remote_configs[remote->get_id()] = remote;
}

void ConfigurationHandler::set_local_config(
    shared_ptr<LocalConfiguration>&& local) {
  unique_lock<shared_mutex> lock(m_local_mutex);
  m_local_config = local;
}

shared_ptr<const LocalConfiguration> ConfigurationHandler::get_local_config()
    const {
  shared_lock<shared_mutex> lock(m_local_mutex);
  return m_local_config;
}

shared_ptr<RemoteConfiguration> ConfigurationHandler::get_remote_config(
    const RemoteId& remote_id) const {
  shared_lock<shared_mutex> lock(m_remote_mutex);
  return m_remote_configs.at(remote_id);
}

bool ConfigurationHandler::remote_exists(const RemoteId& remote_id) {
  shared_lock<shared_mutex> lock(m_remote_mutex);
  return m_remote_configs.find(remote_id) != m_remote_configs.end();
}

size_t ConfigurationHandler::get_remote_count() const {
  shared_lock<shared_mutex> lock(m_remote_mutex);
  return m_remote_configs.size();
}

void ConfigurationHandler::set_server_config(ServerConfig&& server_config){
  m_server_config = move(server_config);
}

ServerConfig ConfigurationHandler::get_server_config() const {
  return m_server_config;
}

CircuitConfig make_circuit_config(const shared_ptr<const LocalConfiguration>& local_config,
                                  const shared_ptr<const RemoteConfiguration>& remote_config){
auto server_config{ConfigurationHandler::cget().get_server_config()};
return {local_config->get_epilink_config(),
  server_config.circuit_directory,
  remote_config->get_matching_mode(),
  server_config.boolean_sharing,
  server_config.use_circuit_conversion};
}

nlohmann::json ConfigurationHandler::make_comparison_config(const RemoteId& remote_id) const {
  nlohmann::json server_config({});
  {
  unique_lock<shared_mutex> local_lock(m_local_mutex);
  auto epi_config = m_local_config->get_epilink_config();
  local_lock.unlock();
  server_config["fields"] = epi_config.fields;
  server_config["exchangeGroups"] = epi_config.exchange_groups;
  server_config["threshold_match"] = epi_config.threshold;
  server_config["threshold_non_match"] = epi_config.tthreshold;
  }
  {
  lock_guard<shared_mutex> remote_lock(m_remote_mutex);
  server_config["matchingMode"] = m_remote_configs.at(remote_id)->get_matching_mode();
  }
  server_config["availableAbyPorts"] = ConnectionHandler::cget().get_free_ports();
  return server_config;
}
bool ConfigurationHandler::compare_configuration(const nlohmann::json& client_config, const RemoteId& remote_id) const{
  nlohmann::json server_config;
  server_config = make_comparison_config(remote_id);
  server_config.erase(server_config.find("availableAbyPorts"));
  return client_config == server_config;
}
}  // namespace sel


