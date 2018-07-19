/**
\file    localconfiguration.h
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
\brief Holds Information about a local connection
*/

#ifndef SEL_LOCALCONFIGURATION_H
#define SEL_LOCALCONFIGURATION_H
#pragma once

#include "databasefetcher.h"
#include "seltypes.h"
#include "resttypes.h"
#include <chrono>
#include "localserver.h"
#include <map>
#include <memory>
#include <string>

#include "authenticationconfig.hpp"
#include "epilink_input.h"
#include "secure_epilinker.h"
#include "nlohmann/json.hpp"

namespace sel {
//class AuthenticationConfig;

class LocalConfiguration {
 public:
  struct AbyInfo{
    size_t default_page_size{25u};
    uint32_t aby_threads{1u};
    e_sharing boolean_sharing{S_YAO};
  };

  LocalConfiguration() = default;
  LocalConfiguration(std::string&& url,
                     std::unique_ptr<AuthenticationConfig> local_auth);

  void add_field(ML_Field field);
  const ML_Field& get_field(const FieldName& fieldname) const;
  std::map<FieldName, ML_Field> get_fields() const;

  void add_exchange_group(IndexSet group);
  std::vector<IndexSet> const& get_exchange_groups() const;

  bool field_exists(const FieldName& fieldname) const;

  void set_data_service(std::string&& url);
  std::string get_data_service() const;

  void set_local_auth(std::unique_ptr<AuthenticationConfig> auth);
  std::string print_auth_type() const;
  AuthenticationConfig const* get_local_authentication() const;

  AbyInfo get_aby_info() const;
  nlohmann::json get_comparison_json() const;

 private:
  std::unique_ptr<AuthenticationConfig> m_local_authentication;
  std::map<FieldName, ML_Field> m_fields;
  std::vector<IndexSet> m_exchange_groups;
  std::string m_data_service_url;
  AbyInfo m_aby_info;
};

void to_json (nlohmann::json&, const ML_Field&);
void from_json(const nlohmann::json&, ML_Field&);

}  // namespace sel

#endif /* end of include guard: SEL_LOCALCONFIGURATION_H */
