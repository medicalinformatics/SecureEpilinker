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

#include "seltypes.h"
#include "linkagejob.h"
#include <memory>
#include <mutex>

namespace sel {
class RemoteConfiguration;
class LocalConfiguration;
class ConnectionHandler;
class DataHandler;
struct AlgorithmConfig;

class ConfigurationHandler {
 public:
   ConfigurationHandler(std::shared_ptr<ConnectionHandler> con_handler) : m_connection_handler(std::move(con_handler)) {}
  std::shared_ptr<const LocalConfiguration> get_local_config() const;
  std::shared_ptr<const AlgorithmConfig> get_algorithm_config() const;
  std::shared_ptr<RemoteConfiguration> get_remote_config(
      const RemoteId&) const;

  void set_local_config(std::shared_ptr<LocalConfiguration>&&);
  void set_algorithm_config(std::shared_ptr<AlgorithmConfig>&&);
  void set_remote_config(std::shared_ptr<RemoteConfiguration>&&);

  size_t get_remote_count() const;

 private:
  std::shared_ptr<LocalConfiguration> m_local_config;
  std::shared_ptr<const AlgorithmConfig> m_algo_config;
  std::map<RemoteId, std::shared_ptr<RemoteConfiguration>>
      m_remote_configs;
  std::shared_ptr<ConnectionHandler> m_connection_handler;
  mutable std::mutex m_local_mutex;
  mutable std::mutex m_remote_mutex;  // Maybe one Mutex per remote? (perf)
  mutable std::mutex m_algo_mutex;
};

EpilinkConfig get_epilink_config(std::shared_ptr<const LocalConfiguration>,
                                    std::shared_ptr<const AlgorithmConfig>);
}  // namespace sel

#endif /* end of include guard: SEL_CONFIGURATIONHANDLER_H */
