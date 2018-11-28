/**
\file    linkagejob.h
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
\brief Holds information and data for one linkage job
*/


#ifndef SEL_LINKAGEJOB_H
#define SEL_LINKAGEJOB_H
#pragma once

#include "seltypes.h"
#include "resttypes.h"
#include "resourcehandler.h"
#include "methodhandler.hpp"
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <map>
#include "epilink_input.h"

namespace restbed {
class Service;
}

namespace sel {
class ConnectionHandler;
class LocalConfiguration;
class RemoteConfiguration;
class ServerHandler;
class SecureEpilinker;

class LinkageJob {
  struct JobPreparation {
    size_t num_records;
    size_t database_size;
    std::shared_ptr<SecureEpilinker> epilinker;
  };
 public:
   LinkageJob();
   LinkageJob(std::shared_ptr<const LocalConfiguration>, std::shared_ptr<const RemoteConfiguration>);
   void set_callback(std::string&& cc);
   void add_data(std::unique_ptr<Records>);
   JobStatus get_status() const;
   void set_status(JobStatus);
   bool is_counting_job() {return m_counting_job;}
   void set_counting_job() {m_counting_job = true;}
   JobId get_id() const;
   RemoteId get_remote_id() const;
   void run_linkage_job();
   void run_matching_job();
   void set_local_config(std::shared_ptr<LocalConfiguration>);
 private:
  JobPreparation prepare_run();
  size_t get_server_nvals(size_t);
  bool perform_callback(const std::string&) const;
#ifdef DEBUG_SEL_REST
  void compute_debugging_result(const Records&);
  void print_data() const;
#endif
  JobId m_id;
  JobStatus m_status{JobStatus::QUEUED};
    std::unique_ptr<Records> m_records;
  std::string m_callback;
  std::shared_ptr<const LocalConfiguration> m_local_config;
  std::shared_ptr<const RemoteConfiguration> m_remote_config;
  bool m_counting_job{false};
};

}  // namespace sel

#endif  // SEL_LINKAGEJOB_H
