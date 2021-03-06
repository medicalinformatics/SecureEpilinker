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
#include "resttypes.h"
#include "seltypes.h"
#include "util.h"
#include "logger.h"
#include "authenticator.h"

using namespace std;
namespace sel {
LocalConfiguration::LocalConfiguration(
    string&& url,
    unique_ptr<AuthenticationConfig> local_auth)
    : m_authenticator(move(local_auth)), m_data_service_url{move(url)} {}

const FieldSpec& LocalConfiguration::get_field(
    const FieldName& fieldname) const {
    return cref(m_epilink_config.fields.at(fieldname));
}

void LocalConfiguration::set_epilink_config(EpilinkConfig config) {
  m_epilink_config = move(config);
}

const EpilinkConfig& LocalConfiguration::get_epilink_config() const {
  return cref(m_epilink_config);
}

const std::map<FieldName, FieldSpec>& LocalConfiguration::get_fields() const {
  return m_epilink_config.fields;
}

vector<IndexSet> const& LocalConfiguration::get_exchange_groups() const {
  return m_epilink_config.exchange_groups;
}

bool LocalConfiguration::field_exists(const FieldName& fieldname) const {
  auto it = m_epilink_config.fields.find(fieldname);
  return it == m_epilink_config.fields.end();
}

void LocalConfiguration::set_data_service(string&& url) {
  m_data_service_url = url;
}

string LocalConfiguration::get_data_service() const {
  return m_data_service_url;
}

void LocalConfiguration::configure_local_authenticator(unique_ptr<AuthenticationConfig> auth) {
  m_authenticator.set_auth_info(move(auth));
}

Authenticator const& LocalConfiguration::get_local_authenticator() const {
  return m_authenticator;
}

void LocalConfiguration::set_local_id(string&& local_id){
  m_local_id = move(local_id);
}

string LocalConfiguration::get_local_id() const {
  return m_local_id;
}


void to_json(nlohmann::json& j, const FieldSpec& f) {
  j = nlohmann::json{
      {"name", f.name},
      {"weight", f.weight},
      {"bitlength", f.bitsize},
      {"comparator",
       (f.comparator == FieldComparator::DICE) ? "dice" : "binary"},
      {"fieldType", ftype_to_str(f.type)}};
}

void from_json(const nlohmann::json& j, FieldSpec& f){
f = FieldSpec{j.at("name").get<string>(), j.at("weight").get<double>(),
                  str_to_fcomp(j.at("comparator").get<string>()),
                  str_to_ftype(j.at("fieldType").get<string>()),
                  j.at("bitlength").get<size_t>()};
}
}  // namespace sel
