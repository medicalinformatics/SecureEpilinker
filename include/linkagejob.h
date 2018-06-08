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

class LinkageJob {
 public:
   LinkageJob();
   LinkageJob(std::shared_ptr<LocalConfiguration>, std::shared_ptr<RemoteConfiguration>);
   void set_callback(CallbackConfig cc) {m_callback = std::move(cc);}
   void add_hw_data_field(const FieldName& fieldname, DataField field, bool);
   void add_bin_data_field(const FieldName& fieldname, DataField field, bool);
   JobStatus get_status() const { return m_status; }
   JobId get_id() const {return m_id;}
   void run_job();
   void set_local_config(std::shared_ptr<LocalConfiguration>);
 private:
   void set_id();
   JobId m_id;
  JobStatus m_status{JobStatus::QUEUED};
  std::map<FieldName, Bitmask> m_hw_data;
  std::map<FieldName, bin_type> m_bin_data;
  std::map<FieldName, bool> m_hw_empty;
  std::map<FieldName, bool> m_bin_empty;
  CallbackConfig m_callback;
  std::shared_ptr<LocalConfiguration> m_local_config;
  std::shared_ptr<RemoteConfiguration> m_parent;
};

}  // namespace sel

#endif  // SEL_LINKAGEJOB_H
