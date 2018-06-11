/**
\file    remoteconfiguration.cpp
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

#include "remoteconfiguration.h"
#include "util.h"

using namespace std;
namespace sel {
RemoteConfiguration::RemoteConfiguration()
    : m_worker_thread(&RemoteConfiguration::run_queued_jobs, this) {}
RemoteConfiguration::RemoteConfiguration(RemoteId c_id)
    : m_connection_id(move(c_id)),
      m_worker_thread(&RemoteConfiguration::run_queued_jobs, this) {}

RemoteConfiguration::~RemoteConfiguration() {
  m_worker_thread.join();
}

void RemoteConfiguration::run_queued_jobs() {
  while (true) {
    if (!m_job_queue.empty()) {
      for (auto& job : m_job_queue) {
        if (job.second->get_status() == JobStatus::QUEUED) {
          job.second->run_job();
        }
        /*
        if(job.get_status() == JobStatus::FINISHED) {
          remove_job(job.get_id());
        }*/
      }
    }
  }
}

void RemoteConfiguration::remove_job(const JobId& j_id) {
  lock_guard<mutex> lock(m_queue_mutex);
  auto job = m_job_queue.find(j_id);
  if (job != m_job_queue.end()) {
    m_job_queue.erase(job);
  }
}

uint16_t RemoteConfiguration::get_remote_port() const {
  auto parts{split(m_connection_profile.url, ':')};
  // {{192.168.1.1},{8080}}
  return stoi(parts.back());
}

string RemoteConfiguration::get_remote_host() const {
  auto parts{split(m_connection_profile.url, ':')};
  // {{192.168.1.1},{8080}}
  return parts.front();
}
}  // namespace sel
