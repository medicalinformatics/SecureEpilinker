/**
\file    localconfiguration.cpp
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

#include "localconfiguration.h"
#include <exception>
#include <thread>
#include "authenticationconfig.hpp"
#include "databasefetcher.h"
#include "epilink_input.h"
#include "fmt/format.h"  // needs to be included before local headers for custom formatters
#include "linkagejob.h"
#include "secure_epilinker.h"
#include "seltypes.h"
#include "resttypes.h"
#include "util.h"
#include "logger.h"

using namespace std;
namespace sel {
LocalConfiguration::LocalConfiguration(
    string&& url,
    unique_ptr<AuthenticationConfig> local_auth)
    : m_local_authentication(move(local_auth)), m_data_service_url{move(url)} {}

void LocalConfiguration::add_field(ML_Field field) {
  FieldName fieldname{field.name};
  m_fields.emplace(move(fieldname), move(field));
}

const ML_Field& LocalConfiguration::get_field(
    const FieldName& fieldname) const {
  try {
    return cref(m_fields.at(fieldname));
  } catch (const exception& e) {
    auto logger{get_default_logger()};
    logger->error("Error in get_field: {}", e.what());
  }
}

std::map<FieldName, ML_Field> LocalConfiguration::get_fields() const {
  return m_fields;
}

void LocalConfiguration::add_exchange_group(IndexSet group) {
  for (const auto& f : group) {
    if (!field_exists(f)) {
      throw runtime_error("Invalid Exchange Group. Field(s) does not exist!");
    }
  }
  m_exchange_groups.emplace_back(move(group));
}

vector<IndexSet> const& LocalConfiguration::get_exchange_groups() const {
  return m_exchange_groups;
}

bool LocalConfiguration::field_exists(const FieldName& fieldname) const {
  auto it = m_fields.find(fieldname);
  return it == m_fields.end() ? false : true;
}

void LocalConfiguration::set_data_service(string&& url) {
  m_data_service_url = url;
}

string LocalConfiguration::get_data_service() const {
  return m_data_service_url;
}

void LocalConfiguration::set_local_auth(unique_ptr<AuthenticationConfig> auth) {
  m_local_authentication = move(auth);
}

string LocalConfiguration::print_auth_type() const {
  return m_local_authentication->print_type();
}

AuthenticationConfig const* LocalConfiguration::get_local_authentication()
    const {
  return m_local_authentication.get();
}

LocalConfiguration::AbyInfo LocalConfiguration::get_aby_info() const {
  return m_aby_info;
}

nlohmann::json LocalConfiguration::get_comparison_json() const {
  nlohmann::json j;
  j["exchangeGroups"] = m_exchange_groups;
  j["fields"] = m_fields;
  return j;
}

void to_json(nlohmann::json& j, const ML_Field& f) {
  j = nlohmann::json{
      {"name", f.name},
      {"weight", f.weight},
      {"bitlength", f.bitsize},
      {"comparator",
       (f.comparator == FieldComparator::NGRAM) ? "nGram" : "binary"},
      {"fieldType", ftype_to_str(f.type)}};
}

void from_json(const nlohmann::json& j, ML_Field& f){
f = ML_Field{j.at("name").get<string>(), j.at("weight").get<double>(),
                  str_to_fcomp(j.at("comparator").get<string>()),
                  str_to_ftype(j.at("fieldType").get<string>()),
                  j.at("bitlength").get<size_t>()};
}
}  // namespace sel
