/**
\file    configurationhandler.h
\author  Tobias Kussel <kussel@cbs.tu-darmstadt.de>
\copyright SEL - Secure EpiLinker
    Copyright (C) 2018 Computational Biology & Simulation Group TU-Darmstadt
    This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License as published
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

#ifndef SEL_CONFIGURATIONHANDLER_H
#define SEL_CONFIGURATIONHANDLER_H
#pragma once

#include "resttypes.h"
#include "linkagejob.h"
#include "circuit_config.h"
#include <memory>
#include <mutex>
#include <shared_mutex>

namespace sel {
class RemoteConfiguration;
class LocalConfiguration;
class ConnectionHandler;
class DataHandler;
struct AlgorithmConfig;

class ConfigurationHandler {
  protected:
    ConfigurationHandler() = default;
 public:
    static ConfigurationHandler& get();
    static ConfigurationHandler const& cget(); // static member funct. can not be const
  std::shared_ptr<const LocalConfiguration> get_local_config() const;
  std::shared_ptr<RemoteConfiguration> get_remote_config(
      const RemoteId&) const;

  void set_local_config(std::shared_ptr<LocalConfiguration>&&);
  void set_remote_config(std::shared_ptr<RemoteConfiguration>&&);
  bool remote_exists(const RemoteId&);
  void set_server_config(ServerConfig&&);
  bool compare_configuration(const nlohmann::json&, const RemoteId&) const;
  nlohmann::json make_comparison_config(const RemoteId&) const;

  size_t get_remote_count() const;
  ServerConfig get_server_config() const;

 private:
  std::shared_ptr<LocalConfiguration> m_local_config;
  std::map<RemoteId, std::shared_ptr<RemoteConfiguration>>
      m_remote_configs;
  ServerConfig m_server_config;
  mutable std::shared_mutex m_local_mutex;
  mutable std::shared_mutex m_remote_mutex;  // Maybe one Mutex per remote? (perf)
};

CircuitConfig make_circuit_config(const std::shared_ptr<const LocalConfiguration>&,
                                  const std::shared_ptr<const RemoteConfiguration>&);
}  // namespace sel

#endif /* end of include guard: SEL_CONFIGURATIONHANDLER_H */
