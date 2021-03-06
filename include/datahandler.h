/**
\file    datahandler.h
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
\brief Class to handle and distribute the database
*/

#ifndef SEL_DATAHANDLER_H
#define SEL_DATAHANDLER_H
#pragma once

#include "resttypes.h"
#include "epilink_input.h"
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#ifdef DEBUG_SEL_REST
#include "epilink_result.hpp"
#include "circuit_config.h"
#endif

namespace sel {

class ConfigurationHandler;
class DatabaseFetcher;

struct ServerData {
  std::shared_ptr<VRecord> data;
  std::shared_ptr<std::vector<std::string>> ids;
  ToDate todate;
  RemoteId local_id;
  RemoteId remote_id;
};

#ifdef DEBUG_SEL_REST
struct Debugger{
  std::optional<Records> client_input;
  std::optional<VRecord> server_input;
  std::optional<sel::CircuitConfig> circuit_config;
  std::vector<Result<CircUnit>> int_result;
  std::vector<Result<double>> double_result;
  bool run;
  bool all_values_set() const;
  void compute_int();
  void compute_double();
  void reset();
};
#endif

class DataHandler {
  DataHandler() = default;
 public:
  static DataHandler& get();
  static DataHandler const& cget();
  std::shared_ptr<const ServerData> get_database() const;
  size_t poll_database(const RemoteId&, bool);
  size_t poll_database_diff();  // TODO(TK) Not implemented yet. Use full update
#ifdef DEBUG_SEL_REST
  Debugger* get_epilink_debug() { return m_epilink_debug;}
#endif
 private:
  mutable std::mutex m_db_mutex;
  std::shared_ptr<const ServerData> m_database;
  std::unique_ptr<DatabaseFetcher> m_database_fetcher;
#ifdef DEBUG_SEL_REST
  Debugger* m_epilink_debug{new Debugger};
#endif
};

}  // namespace sel

#endif /* end of include guard: SEL_DATAHANDLER_H */
