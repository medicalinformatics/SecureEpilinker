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
#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <set>

#include "authenticationconfig.hpp"
#include "epilink_input.h"
#include "epilink_input.h"

namespace sel {
//class AuthenticationConfig;

class LocalConfiguration {
 public:
  LocalConfiguration() = default;
  LocalConfiguration(std::string&& url,
                     std::unique_ptr<AuthenticationConfig> local_auth);

  void add_field(ML_Field field);

  const ML_Field& get_field(const FieldName& fieldname);
  std::vector<double> get_weights(FieldComparator) const;
  void add_exchange_group(std::set<FieldName> group);

  bool field_exists(const FieldName& fieldname);
  bool field_hw_exists(const FieldName& fieldname);
  bool field_bin_exists(const FieldName& fieldname);
  void set_algorithm_config(AlgorithmConfig aconfig);

  void set_data_service(std::string&& url);
  void set_local_auth(std::unique_ptr<AuthenticationConfig> auth);
  void poll_data();
  void poll_differential_data();
  std::vector<std::set<FieldName>> const& get_exchange_group(FieldComparator) const;
  std::vector<std::set<size_t>> get_exchange_group_indices(FieldComparator) const;

  void run_comparison();
  std::string print_auth_type() const {return m_local_authentication->print_type();}
  unsigned get_bloom_length() const {return m_algorithm.bloom_length;}
  AlgorithmConfig const& get_algorithm_config() const {return m_algorithm;}

 private:
  void start_aby_server() const;
  std::unique_ptr<AuthenticationConfig> m_local_authentication;
  std::map<FieldName, ML_Field> m_bin_fields;
  std::map<FieldName, ML_Field> m_hw_fields;
  std::vector<std::set<FieldName>> m_hw_exchange_groups;
  std::vector<std::set<FieldName>> m_bin_exchange_groups;
  AlgorithmConfig m_algorithm;
  std::string m_data_service_url;
  std::map<FieldName, std::vector<Bitmask>> m_hw_data;
  std::map<FieldName, std::vector<bool>> m_hw_empty;
  std::map<FieldName, VCircUnit> m_bin_data;
  std::map<FieldName, std::vector<bool>> m_bin_empty;
  std::vector<std::map<std::string,std::string>> m_ids;
  size_t m_todate;
  std::unique_ptr<DatabaseFetcher> m_database_fetcher{std::make_unique<DatabaseFetcher>(this)};
};

}  // namespace sel

#endif /* end of include guard: SEL_LOCALCONFIGURATION_H */
