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
#include <map>
#include <memory>
#include <string>

#include "authenticationconfig.hpp"
#include "epilink_input.h"
#include "nlohmann/json.hpp"

namespace sel {
//class AuthenticationConfig;

class LocalConfiguration {
 public:
  LocalConfiguration() = default;
  LocalConfiguration(std::string&& url,
                     std::unique_ptr<AuthenticationConfig> local_auth);

  const ML_Field& get_field(const FieldName& fieldname) const;
  const std::map<FieldName, ML_Field>& get_fields() const;

  std::vector<IndexSet> const& get_exchange_groups() const;

  void set_epilink_config(EpilinkConfig);
  const EpilinkConfig& get_epilink_config() const;

  bool field_exists(const FieldName& fieldname) const;

  void set_data_service(std::string&& url);
  std::string get_data_service() const;

  void set_local_auth(std::unique_ptr<AuthenticationConfig> auth);
  std::string print_auth_type() const;
  AuthenticationConfig const* get_local_authentication() const;

  void set_local_id(std::string&&);
  std::string get_local_id() const;

 private:
  std::unique_ptr<AuthenticationConfig> m_local_authentication;
  std::map<FieldName, ML_Field> m_fields;
  std::vector<IndexSet> m_exchange_groups;
  std::string m_data_service_url;
  std::string m_local_id;
  EpilinkConfig m_epilink_config;
};

void to_json (nlohmann::json&, const ML_Field&);
void from_json(const nlohmann::json&, ML_Field&);

}  // namespace sel

#endif /* end of include guard: SEL_LOCALCONFIGURATION_H */
