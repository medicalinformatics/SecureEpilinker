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
#include <set>
#include <thread>
#include "authenticationconfig.hpp"
#include "databasefetcher.h"
#include "epilink_input.h"
#include "fmt/format.h"
#include "secure_epilinker.h"
#include "seltypes.h"

using namespace sel;
sel::LocalConfiguration::LocalConfiguration(
    std::string&& url,
    std::unique_ptr<sel::AuthenticationConfig> local_auth)
    : m_local_authentication(std::move(local_auth)), m_data_service_url{std::move(url)} { }

void sel::LocalConfiguration::add_field(sel::ML_Field field) {
  sel::FieldName fieldname{field.name};
  if (field.comparator == sel::FieldComparator::NGRAM) {
    m_hw_fields.emplace(std::move(fieldname), std::move(field));
  } else {
    m_bin_fields.emplace(std::move(fieldname), std::move(field));
  }
}

const sel::ML_Field& LocalConfiguration::get_field(
    const sel::FieldName& fieldname) {
  if (field_hw_exists(fieldname)) {
    return std::cref(m_hw_fields[fieldname]);
  } else if (field_bin_exists(fieldname)) {
    return std::cref(m_bin_fields[fieldname]);
  } else {
    throw std::runtime_error("Field \"" + fieldname + "\" does not exist.");
  }
}

std::vector<double> sel::LocalConfiguration::get_weights(
    sel::FieldComparator comparator) const {
  std::vector<double> tempvec;
  if (comparator == sel::FieldComparator::NGRAM) {
    tempvec.reserve(m_hw_fields.size());
    for (const auto& p : m_hw_fields) {
      tempvec.emplace_back(p.second.weight);
    }
  } else {
    tempvec.reserve(m_bin_fields.size());
    for (const auto& p : m_bin_fields) {
      tempvec.emplace_back(p.second.weight);
    }
  }
  return tempvec;
}

void LocalConfiguration::add_exchange_group(std::set<sel::FieldName> group) {
  bool hw{false};
  bool bin{false};
  for (const auto& f : group) {
    if (!field_exists(f)) {
      throw std::runtime_error(
          "Invalid Exchange Group. Field(s) does not exist!");
    }
    if (get_field(f).comparator == sel::FieldComparator::NGRAM) {
      hw = true;
    } else {
      bin = true;
    }
  }
  if (!(!hw != !bin)) {
    throw std::runtime_error("Mixed Exchangegroups are not implemented");
  }
  if (hw) {
    m_hw_exchange_groups.emplace_back(std::move(group));
  } else {
    m_bin_exchange_groups.emplace_back(std::move(group));
  }
}

bool LocalConfiguration::field_exists(const sel::FieldName& fieldname) {
  auto hw_it = m_hw_fields.find(fieldname);
  if (hw_it != m_hw_fields.end()) {
    return true;
  }
  auto bin_it = m_bin_fields.find(fieldname);
  if (bin_it != m_bin_fields.end()) {
    return true;
  }

  return false;
}

bool LocalConfiguration::field_hw_exists(const sel::FieldName& fieldname) {
  auto hw_it = m_hw_fields.find(fieldname);
  return (hw_it != m_hw_fields.end()) ? true : false;
}

bool LocalConfiguration::field_bin_exists(const sel::FieldName& fieldname) {
  auto bin_it = m_bin_fields.find(fieldname);
  return (bin_it != m_bin_fields.end()) ? true : false;
}

void LocalConfiguration::set_algorithm_config(AlgorithmConfig aconfig) {
  m_algorithm = aconfig;
}

void LocalConfiguration::set_data_service(std::string&& url) {
  m_data_service_url = url;
}

void LocalConfiguration::set_local_auth(
    std::unique_ptr<AuthenticationConfig> auth) {
  m_local_authentication = std::move(auth);
}

void LocalConfiguration::poll_data() {
  m_database_fetcher->set_url(m_data_service_url);
  m_database_fetcher->set_page_size(25u);  // TODO(TK): Magic number raus!
  auto&& data{m_database_fetcher->fetch_data(m_local_authentication.get())};
  m_todate = data.todate;
  m_hw_data = std::move(data.hw_data);
  m_bin_data = std::move(data.bin_data);
  m_hw_empty = std::move(data.hw_empty);
  m_bin_empty = std::move(data.bin_empty);
  m_ids = std::move(data.ids);
}

std::vector<std::set<FieldName>> const&
sel::LocalConfiguration::get_exchange_group(sel::FieldComparator comp) const {
  return (comp == sel::FieldComparator::NGRAM) ? m_hw_exchange_groups
                                               : m_bin_exchange_groups;
}

std::vector<std::set<size_t>>
sel::LocalConfiguration::get_exchange_group_indices(
    sel::FieldComparator comp) const {
  auto& exchange_groups{(comp == sel::FieldComparator::NGRAM)
                           ? m_hw_exchange_groups
                           : m_bin_exchange_groups};
  auto& fields{(comp == sel::FieldComparator::NGRAM) ? m_hw_fields
                                                     : m_bin_fields};
  std::vector<std::set<size_t>> tempvec;
  for (auto& exchange_set : exchange_groups) {
    std::set<size_t> tempset;
    for(auto& exchange_field : exchange_set){
    auto field_iterator{fields.find(exchange_field)};
    if (field_iterator == fields.end()) {
      throw std::runtime_error("Invalid Exchange Group!");
    }
    tempset.emplace(
        static_cast<size_t>(std::distance(fields.begin(), field_iterator)));
    }
    tempvec.emplace_back(std::move(tempset));
  }
  return tempvec;
}

void LocalConfiguration::run_comparison() {
  // Fill Member variables
  poll_data();
  // Get Weights and calcualte # of Records
  sel::v_weight_type hw_weights{get_weights(sel::FieldComparator::NGRAM)};
  sel::v_weight_type bin_weights{get_weights(sel::FieldComparator::BINARY)};
  const size_t nvals{m_hw_data.begin()->second.size()}; // assuming each record has hw and bin
  fmt::print("Number of records: {}\n", nvals);
  // make data and empty map to vectors
  std::vector<std::vector<sel::bitmask_type>> hw_data;
  std::vector<sel::v_bin_type> bin_data;
  hw_data.reserve(m_hw_fields.size());
  bin_data.reserve(m_bin_fields.size());
  std::vector<std::vector<bool>> hw_empty;
  std::vector<std::vector<bool>> bin_empty;
  hw_empty.reserve(m_hw_fields.size());
  bin_empty.reserve(m_bin_fields.size());
  for (auto& field : m_hw_data) {
    hw_data.emplace_back(field.second);
  }
  for (auto& field : m_bin_data) {
    bin_data.emplace_back(field.second);
  }
  for (auto& field : m_hw_empty) {
    hw_empty.emplace_back(field.second);
  }
  for (auto& field : m_bin_empty) {
    bin_empty.emplace_back(field.second);
  }
  try {
    sel::EpilinkConfig epilink_config{std::move(hw_weights), std::move(bin_weights),
         get_exchange_group_indices(sel::FieldComparator::NGRAM),
         get_exchange_group_indices(sel::FieldComparator::BINARY),
         m_algorithm.bloom_length, m_algorithm.threshold_match,
         m_algorithm.threshold_non_match};
    sel::SecureEpilinker aby_server_party{
        {SERVER, S_BOOL, "127.0.0.1", 8888, 1},epilink_config};
    aby_server_party.build_circuit(nvals);
    aby_server_party.run_setup_phase();
    sel::EpilinkServerInput server_input{hw_data, bin_data, hw_empty, bin_empty};
    fmt::print("Server running!\n");
    //const auto server_share{aby_server_party.run_as_server(server_input)};
    //fmt::print("Server Share: {}\n", server_share);
  } catch (const std::exception& e) {
    fmt::print(stderr, "Error running MPC server: {}\n", e.what());
  }
  // Send ABY Share and IDs to Linkage Server
}
