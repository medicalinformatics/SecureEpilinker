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
#include <thread>
#include "authenticationconfig.hpp"
#include "databasefetcher.h"
#include "fmt/format.h"
#include "seltypes.h"
#include <exception>

using namespace sel;
sel::LocalConfiguration::LocalConfiguration(
    std::string&& url,
    std::unique_ptr<sel::AuthenticationConfig> local_auth)
    : m_local_authentication(std::move(local_auth)), m_data_service(url) {}

void sel::LocalConfiguration::add_field(sel::ML_Field field) {
  sel::FieldName fieldname{field.name};
  m_fields.emplace(std::move(fieldname), std::move(field));
}

const sel::ML_Field& LocalConfiguration::get_field(
    const sel::FieldName& fieldname) {
  if (field_exists(fieldname)) {
    return std::cref(m_fields[fieldname]);
  } else {
    throw std::runtime_error("Field \"" + fieldname + "\" does not exist.");
  }
}

void LocalConfiguration::add_exchange_group(std::vector<sel::FieldName>& group) {
  std::vector<sel::FieldName> tempgroup;

  for (const auto& f : group) {
    if (field_exists(f)) {
      tempgroup.emplace_back(f);
    } else {
      throw std::runtime_error(
          "Invalid Exchange Group. Field(s) does not exist!");
    }
  }
  m_exchange_groups.emplace_back(std::move(tempgroup));
}

bool LocalConfiguration::field_exists(const sel::FieldName& fieldname) {
  auto it = m_fields.find(fieldname);
  return (it != m_fields.end());
}

void LocalConfiguration::set_algorithm_config(AlgorithmConfig aconfig) {
  m_algorithm = aconfig;
}

void LocalConfiguration::set_data_service_url(std::string&& url) {
  m_data_service = std::move(url);
}

void LocalConfiguration::set_local_auth(
    std::unique_ptr<AuthenticationConfig> auth) {
  m_local_authentication = std::move(auth);
}

void LocalConfiguration::poll_data() {
  m_database_fetcher->set_url(m_data_service);
  m_database_fetcher->set_page_size(25u);  // TODO(TK): Magic number raus!
  auto&& data{m_database_fetcher->fetch_data(m_local_authentication.get())};
  m_todate = data.todate;
  m_data = std::move(data.data);
  m_ids = std::move(data.ids);
}

void LocalConfiguration::run_comparison() {
  poll_data();
  std::vector<std::vector<DataField>> aby_data;
  std::vector<double> weights;
  aby_data.reserve(m_data.size());
  for (auto& field : m_data) {
    aby_data.emplace_back(field.second);
    weights.emplace_back(get_field(field.first).weight);
  }
  try{
    run_aby(0);
  } catch (const std::exception& e){
    fmt::print(stderr, "Error running MPC server: {}\n", e.what());
  }
  // Start ABY Server it gets m_data and weights
  // Send ABY Share and IDs to Linkage Server
}
