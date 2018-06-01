/**
\file    connectionhandler.h
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
\brief Holds the connections to every remote host and dispatches ABY
computations
*/

#ifndef SEL_CONNECTIONHANDLER_H
#define SEL_CONNECTIONHANDLER_H
#pragma once

#include "seltypes.h"
#include "localconfiguration.h"
#include <memory>
#include <unordered_map>
#include <mutex>

namespace restbed {
class Service;
}

namespace sel {
class RemoteConfiguration;
class LinkageJob;
class AuthenticationConfig;
//class LocalConfiguration;

class ConnectionHandler {
  /**
   * Handle connection configurations and jobs. Dispatch ABY computations
   */
 public:
  ConnectionHandler(restbed::Service* service);

  void upsert_connection(std::shared_ptr<RemoteConfiguration> connection);

  size_t num_connections() const;

  void set_local_configuration(std::unique_ptr<LocalConfiguration>&& l_conf);

  bool connection_exists(const RemoteId& c_id) const;

  void dispatch_job();  // TODO(TK): Empty function

  const ML_Field& get_field(const FieldName& name);

  std::shared_ptr<restbed::Service> get_service() const;

  void add_job(const RemoteId& remote_id, std::shared_ptr<LinkageJob> job);

  std::pair<
      std::unordered_map<JobId, std::shared_ptr<LinkageJob>>::iterator,
      std::unordered_map<JobId, std::shared_ptr<LinkageJob>>::iterator>
  find_job(const JobId& id);

  void initiate_comparison() { 
    std::lock_guard<std::mutex> lock(m_local_data_mutex);
    m_local_configuration->run_comparison();
  };

 private:
  void insert_connection(RemoteId&& remote_id,
                         std::shared_ptr<RemoteConfiguration> connection);

  void update_connection(const RemoteId& remote_id,
                         std::shared_ptr<RemoteConfiguration> connection);

  std::unordered_map<RemoteId, std::shared_ptr<RemoteConfiguration>>
      m_connections;
  std::unique_ptr<LocalConfiguration> m_local_configuration;
  std::unordered_map<JobId, RemoteId> m_job_id_to_remote_id;
  std::shared_ptr<restbed::Service> m_service;
  std::mutex m_local_data_mutex;
};

}  // Namespace sel

#endif  // SEL_CONNECTIONHANDLER_
