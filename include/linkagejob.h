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

namespace restbed {
class Service;
}

namespace sel {
class ConnectionHandler;

class LinkageJob {
 public:
   LinkageJob();
   void set_callback(CallbackConfig cc) {m_callback = std::move(cc);}
   void add_data_field(const FieldName& fieldname, DataField field) {m_data.emplace(fieldname, std::move(field));}
   JobStatus get_status() const { return m_status; }
   JobId get_id() const {return m_id;}
   void run_job();
 private:
   JobId m_id;
  JobStatus m_status{JobStatus::QUEUED};
  std::unordered_map<FieldName, std::variant<int, double, std::string, std::vector<uint8_t>>>
      m_data;
  CallbackConfig m_callback;
};

}  // namespace sel

#endif  // SEL_LINKAGEJOB_H
