/**
\file    databasefetcher.h
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
\brief Queries the database for patient data
*/

#ifndef SEL_DATABASEFETCHER_H
#define SEL_DATABASEFETCHER_H

#include <map>
#include <string>
#include <vector>
#include "datahandler.h"
#include "epilink_input.h"
#include "nlohmann/json.hpp"
#include "resttypes.h"

namespace spdlog {
class logger;
}

namespace sel {
class LocalConfiguration;

class DatabaseFetcher {
 public:
  ServerData fetch_data();
  DatabaseFetcher(std::shared_ptr<const LocalConfiguration> local_conf,
                  std::shared_ptr<const AlgorithmConfig> algo_conf,
                  std::string url,
                  AuthenticationConfig const* l_auth);

  DatabaseFetcher(std::shared_ptr<const LocalConfiguration> local_conf,
                  std::shared_ptr<const AlgorithmConfig> algo_conf,
                  std::string url,
                  AuthenticationConfig const* l_auth,
                  size_t page_size);
  void set_url(const std::string& url) { m_url = url; }
  void set_page_size(unsigned size) { m_page_size = size; }
  size_t get_todate() const { return m_todate; }

 private:
  nlohmann::json get_next_page() const;
  nlohmann::json request_page(const std::string& url) const;
  void get_page_data(const nlohmann::json&);
  std::map<FieldName, VFieldEntry> m_data;
  std::vector<std::string> m_ids;
  std::string m_next_page;
  size_t m_todate;
  RemoteId m_local_id;
  RemoteId m_remote_id;
  std::string m_url;
  std::shared_ptr<const LocalConfiguration> m_local_config;
  std::shared_ptr<const AlgorithmConfig> m_algo_config;
  AuthenticationConfig const* m_local_authentication;
  unsigned m_page_size{25u};
  unsigned m_last_page{1u};
  unsigned m_page{1u};
  std::shared_ptr<spdlog::logger> m_logger;
};

}  // namespace sel

#endif /* end of include guard: SEL_DATABASEFETCHER_H */
