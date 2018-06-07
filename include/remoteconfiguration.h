/**
\file    remoteconfiguration.h
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
\brief Holds Information about a remote connection
*/

#ifndef SEL_REMOTECONFIGURATION_H
#define SEL_REMOTECONFIGURATION_H
#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "authenticationconfig.hpp"
#include "linkagejob.h"
#include "seltypes.h"

namespace sel {

class RemoteConfiguration {
  /**
   * Stores secureEpiLinker configuration for one connection
   */
 public:
  RemoteConfiguration();
  RemoteConfiguration(RemoteId c_id);
  ~RemoteConfiguration();
  void set_connection_profile(ConnectionConfig cconfig) {
    m_connection_profile = std::move(cconfig);
  }
  void set_linkage_service(ConnectionConfig cconfig) {
    m_linkage_service = std::move(cconfig);
  }
  void add_job(std::shared_ptr<LinkageJob> job) {
    auto id(job->get_id());
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_job_queue.emplace(std::move(id), std::move(job));
  }

  std::pair<std::unordered_map<JobId, std::shared_ptr<LinkageJob>>::iterator,
            std::unordered_map<JobId, std::shared_ptr<LinkageJob>>::iterator>
  find_job(const JobId& id) {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    return std::make_pair(m_job_queue.find(id), m_job_queue.end());
  }

  JobId get_id() const { return m_connection_id; }

  void run_setup_phase();  // TODO(TK): Not implemented in ABY
  uint16_t get_remote_port() const;
  std::string get_remote_host() const;
 protected:
 private:
  void run_queued_jobs();
  void remove_job(const std::string& j_id);
  RemoteId m_connection_id;
  std::unordered_map<JobId, std::shared_ptr<LinkageJob>> m_job_queue;
  ConnectionConfig m_connection_profile;
  ConnectionConfig m_linkage_service;
  std::mutex m_queue_mutex;
  std::thread m_worker_thread;
};

}  // Namespace sel

#endif  // SEL_REMOTECONFIGURATION_H
