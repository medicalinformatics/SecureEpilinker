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
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "secure_epilinker.h"

namespace sel {
class ConfigurationHandler;
class DatabaseFetcher;

struct ServerData {
  std::map<FieldName, VFieldEntry> data;
  std::vector<std::map<std::string, std::string>> ids;
  ToDate todate;
};
class DataHandler {
 public:
  void set_config_handler(std::shared_ptr<ConfigurationHandler>);
  std::shared_ptr<const ServerData> get_database() const;
  size_t poll_database();
  size_t poll_database_diff();  // TODO(TK) Not implemented yet. Use full update
 private:
  mutable std::mutex m_db_mutex;
  std::shared_ptr<const ServerData> m_database;
  std::unique_ptr<DatabaseFetcher> m_database_fetcher;
  std::shared_ptr<const ConfigurationHandler> m_config_handler;
};

}  // namespace sel

#endif /* end of include guard: SEL_DATAHANDLER_H */
