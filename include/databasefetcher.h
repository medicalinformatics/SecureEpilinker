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
#include "nlohmann/json.hpp"
#include "seltypes.h"
#include "epilink_input.h"

namespace sel {
class LocalConfiguration;

struct PollData {
  size_t todate;
  std::map<FieldName, std::vector<Bitmask>> hw_data;
  std::map<FieldName, VCircUnit> bin_data;
  std::map<FieldName, std::vector<bool>> hw_empty;
  std::map<FieldName, std::vector<bool>> bin_empty;
  std::vector<std::map<std::string, std::string>> ids;
};

class DatabaseFetcher {
 public:
  PollData fetch_data(AuthenticationConfig* l_auth);
  DatabaseFetcher(LocalConfiguration* const parent, const std::string& url)
      : m_url(url), m_parent(parent) {}
  DatabaseFetcher(LocalConfiguration* const parent) : m_parent(parent) {}

  void set_url(const std::string& url) { m_url = url; }
  void set_page_size(unsigned size) { m_page_size = size; }
  size_t get_todate() const { return m_todate; }

 private:
  nlohmann::json get_next_page(AuthenticationConfig* l_auth) const;
  nlohmann::json request_page(const std::string& url,
                              AuthenticationConfig* l_auth) const;
  void get_page_data(const nlohmann::json&);
  std::map<FieldName, std::vector<Bitmask>> m_hw_data;
  std::map<FieldName, sel::VCircUnit> m_bin_data;
  std::map<FieldName, std::vector<bool>> m_hw_empty;
  std::map<FieldName, std::vector<bool>> m_bin_empty;
  std::vector<std::map<std::string, std::string>> m_ids;
  unsigned m_page_size{25u};
  unsigned m_last_page{1u};
  unsigned m_page{1u};
  std::string m_next_page;
  size_t m_todate;
  std::string m_url;
  LocalConfiguration* const m_parent;
};

}  // namespace sel

#endif /* end of include guard: SEL_DATABASEFETCHER_H */
