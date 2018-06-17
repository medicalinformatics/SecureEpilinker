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

#include <exception>
#include <set>
#include <thread>
#include "fmt/format.h" // needs to be included before local headers for custom formatters
#include "localconfiguration.h"
#include "authenticationconfig.hpp"
#include "databasefetcher.h"
#include "epilink_input.h"
#include "secure_epilinker.h"
#include "seltypes.h"
#include "util.h"

using namespace std;
namespace sel {
LocalConfiguration::LocalConfiguration(
    string&& url,
    unique_ptr<AuthenticationConfig> local_auth)
    : m_local_authentication(move(local_auth)), m_data_service_url{move(url)} {}

void LocalConfiguration::add_field(ML_Field field) {
  FieldName fieldname{field.name};
  if (field.comparator == FieldComparator::NGRAM) {
    m_hw_fields.emplace(move(fieldname), move(field));
  } else {
    m_bin_fields.emplace(move(fieldname), move(field));
  }
}

const ML_Field& LocalConfiguration::get_field(const FieldName& fieldname) {
  if (field_hw_exists(fieldname)) {
    return cref(m_hw_fields[fieldname]);
  } else if (field_bin_exists(fieldname)) {
    return cref(m_bin_fields[fieldname]);
  } else {
    throw runtime_error("Field \"" + fieldname + "\" does not exist.");
  }
}

vector<double> LocalConfiguration::get_weights(
    FieldComparator comparator) const {
  vector<double> tempvec;
  if (comparator == FieldComparator::NGRAM) {
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

void LocalConfiguration::add_exchange_group(set<FieldName> group) {
  bool hw{false};
  bool bin{false};
  for (const auto& f : group) {
    if (!field_exists(f)) {
      throw runtime_error("Invalid Exchange Group. Field(s) does not exist!");
    }
    if (get_field(f).comparator == FieldComparator::NGRAM) {
      hw = true;
    } else {
      bin = true;
    }
  }
  if (!(!hw != !bin)) {
    throw runtime_error("Mixed Exchangegroups are not implemented");
  }
  if (hw) {
    m_hw_exchange_groups.emplace_back(move(group));
  } else {
    m_bin_exchange_groups.emplace_back(move(group));
  }
}

bool LocalConfiguration::field_exists(const FieldName& fieldname) {
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

bool LocalConfiguration::field_hw_exists(const FieldName& fieldname) {
  auto hw_it = m_hw_fields.find(fieldname);
  return (hw_it != m_hw_fields.end()) ? true : false;
}

bool LocalConfiguration::field_bin_exists(const FieldName& fieldname) {
  auto bin_it = m_bin_fields.find(fieldname);
  return (bin_it != m_bin_fields.end()) ? true : false;
}

void LocalConfiguration::set_algorithm_config(AlgorithmConfig aconfig) {
  m_algorithm = aconfig;
}

void LocalConfiguration::set_data_service(string&& url) {
  m_data_service_url = url;
}

void LocalConfiguration::set_local_auth(unique_ptr<AuthenticationConfig> auth) {
  m_local_authentication = move(auth);
}

void LocalConfiguration::poll_data() {
  m_database_fetcher->set_url(m_data_service_url);
  m_database_fetcher->set_page_size(25u);  // TODO(TK): Magic number raus!
  auto&& data{m_database_fetcher->fetch_data(m_local_authentication.get())};
  m_todate = data.todate;
  m_hw_data = move(data.hw_data);
  m_bin_data = move(data.bin_data);
  m_hw_empty = move(data.hw_empty);
  m_bin_empty = move(data.bin_empty);
  m_ids = move(data.ids);
}

vector<set<FieldName>> const& LocalConfiguration::get_exchange_group(
    FieldComparator comp) const {
  return (comp == FieldComparator::NGRAM) ? m_hw_exchange_groups
                                          : m_bin_exchange_groups;
}

vector<set<size_t>> LocalConfiguration::get_exchange_group_indices(
    FieldComparator comp) const {
  auto& exchange_groups{(comp == FieldComparator::NGRAM)
                            ? m_hw_exchange_groups
                            : m_bin_exchange_groups};
  auto& fields{(comp == FieldComparator::NGRAM) ? m_hw_fields : m_bin_fields};
  vector<set<size_t>> tempvec;
  for (auto& exchange_set : exchange_groups) {
    set<size_t> tempset;
    for (auto& exchange_field : exchange_set) {
      auto field_iterator{fields.find(exchange_field)};
      if (field_iterator == fields.end()) {
        throw runtime_error("Invalid Exchange Group!");
      }
      tempset.emplace(
          static_cast<size_t>(distance(fields.begin(), field_iterator)));
    }
    tempvec.emplace_back(move(tempset));
  }
  return tempvec;
}

void LocalConfiguration::run_comparison() {
  // Fill Member variables
  poll_data();
  // Get Weights and calcualte # of Records
  VWeight hw_weights{get_weights(FieldComparator::NGRAM)};
  VWeight bin_weights{get_weights(FieldComparator::BINARY)};
  const size_t nvals{
      m_hw_data.begin()->second.size()};  // assuming each record has hw and bin
  fmt::print("Number of records: {}\n", nvals);
  // make data and empty map to vectors
  vector<vector<Bitmask>> hw_data;
  vector<VCircUnit> bin_data;
  hw_data.reserve(m_hw_fields.size());
  bin_data.reserve(m_bin_fields.size());
  vector<vector<bool>> hw_empty;
  vector<vector<bool>> bin_empty;
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
    fmt::print("Input remote host (for now!)\n");
    string host;
    cin >> host;
    fmt::print("Input remote port (argh, fix this!)\n");
    uint16_t port;
    cin >> port;

    SecureEpilinker::ABYConfig aby_config{SERVER, S_BOOL, host, port, 1};
    EpilinkConfig epilink_config{
      { // weights
        {FieldComparator::NGRAM, move(hw_weights)},
        {FieldComparator::BINARY, move(bin_weights)}
      },
      { // exchange groups
        {FieldComparator::NGRAM,
          get_exchange_group_indices(FieldComparator::NGRAM)},
        {FieldComparator::BINARY,
          get_exchange_group_indices(FieldComparator::BINARY)}
      },
      m_algorithm.bloom_length,
      m_algorithm.threshold_match,
      m_algorithm.threshold_non_match
    }; // epilink_config

    SecureEpilinker aby_server_party{aby_config, epilink_config};
    aby_server_party.build_circuit(nvals);
    aby_server_party.run_setup_phase();
    EpilinkServerInput server_input{hw_data, bin_data, hw_empty, bin_empty};
    fmt::print("Server running\n{}\n{}\n{}\n",
        aby_config, epilink_config, server_input);
    const auto server_share{aby_server_party.run_as_server(server_input)};
    fmt::print("Server result:\n{}\n", server_share);
    fmt::print("IDs (in Order):\n");
    for (size_t i = 0; i != m_ids.size(); ++i) {
      fmt::print("{} IDs: ", i);
      for (const auto& m : m_ids[i]) {
        fmt::print("{} - {}; ", m.first, m.second);
      }
      fmt::print("\n");
    }
  } catch (const exception& e) {
    fmt::print(stderr, "Error running MPC server: {}\n", e.what());
  }
  // Send ABY Share and IDs to Linkage Server
}
}  // namespace sel
