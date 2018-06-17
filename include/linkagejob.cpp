/**
\file    linkagejob.cpp
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
\brief Holds information and data for one linkage job
*/

#include "linkagejob.h"
#include <chrono>
#include <exception>
#include <map>
#include <random>
#include <string>
#include <variant>
#include <vector>
#include "fmt/format.h"
#include "localconfiguration.h"
#include "seltypes.h"

#include "epilink_input.h"
#include "remoteconfiguration.h"
#include "secure_epilinker.h"
#include "util.h"

#include <iostream>

using namespace std;
namespace sel {
void LinkageJob::set_id() {
  const auto timestamp{chrono::system_clock::now().time_since_epoch()};
  m_id =
      to_string(chrono::duration_cast<chrono::milliseconds>(timestamp).count());
  random_shuffle(m_id.begin(), m_id.end());
}

LinkageJob::LinkageJob() {
  set_id();
}

LinkageJob::LinkageJob(shared_ptr<LocalConfiguration> l_conf,
                       shared_ptr<RemoteConfiguration> parent)
    : m_local_config(move(l_conf)), m_parent(parent) {
  set_id();
}

void LinkageJob::add_hw_data_field(const FieldName& fieldname,
                                   DataField datafield,
                                   bool empty) {
  assert(holds_alternative<Bitmask>(datafield));
  m_hw_data.emplace(fieldname, get<Bitmask>(datafield));
  m_hw_empty.emplace(fieldname, empty);
}

void LinkageJob::add_bin_data_field(const FieldName& fieldname,
                                    DataField datafield,
                                    bool empty) {
  CircUnit tempfield;
  if (holds_alternative<int>(datafield)) {
    tempfield = static_cast<CircUnit>(get<int>(datafield));
  } else if (holds_alternative<double>(datafield)) {
    tempfield = hash<double>{}(get<double>(datafield));
  } else if (holds_alternative<string>(datafield)) {
    tempfield = hash<string>{}(get<string>(datafield));
  } else {
    throw runtime_error("Invalid Binary Comparison");
    tempfield = 0;
  }
  m_bin_empty.emplace(fieldname, empty);
  m_bin_data.emplace(fieldname, tempfield);
}

void LinkageJob::run_job() {
  m_status = JobStatus::RUNNING;

  // Prepare Data, weights and exchange groups

  const auto& algorithm_config{m_local_config->get_algorithm_config()};
  fmt::print("Job {} started\n", m_id);

  vector<Bitmask> hw_data;
  VCircUnit bin_data;
  hw_data.reserve(m_hw_data.size());
  bin_data.reserve(m_bin_data.size());

  vector<bool> hw_empty;
  vector<bool> bin_empty;
  hw_empty.reserve(m_hw_empty.size());
  bin_empty.reserve(m_bin_empty.size());

  for (const auto& field : m_hw_data) {
    hw_data.emplace_back(field.second);
  }
  for (const auto& field : m_bin_data) {
    bin_data.emplace_back(field.second);
  }
  for (const auto& field : m_hw_empty) {
    hw_empty.emplace_back(field.second);
  }
  for (const auto& field : m_bin_empty) {
    bin_empty.emplace_back(field.second);
  }

  VWeight hw_weights{m_local_config->get_weights(FieldComparator::NGRAM)};
  VWeight bin_weights{
      m_local_config->get_weights(FieldComparator::BINARY)};

  // TODO(TK): To get: remote_ip, remote_port, nvals,
  // threads

  try {
    // Construct ABY Client
    // FIXME(TK): Magicnumbers
    const e_sharing booleantype{S_BOOL};
    const uint32_t nthreads{1};
    size_t nvals;
    fmt::print(
        "Please insert number of records (no worries, this is only "
        "temporary)\n");
    cin >> nvals;

    SecureEpilinker::ABYConfig aby_config{
        CLIENT, booleantype, m_parent->get_remote_host(),
        m_parent->get_remote_port(), nthreads};
    EpilinkConfig epi_config{
      { // weights
        {FieldComparator::NGRAM, hw_weights},
        {FieldComparator::BINARY, bin_weights}
      },
      { // exchange groups
        {FieldComparator::NGRAM,
          m_local_config->get_exchange_group_indices(FieldComparator::NGRAM)},
        {FieldComparator::BINARY,
          m_local_config->get_exchange_group_indices(FieldComparator::BINARY)}
      },
      algorithm_config.bloom_length,
      algorithm_config.threshold_match,
      algorithm_config.threshold_non_match
    }; // epi_config

    SecureEpilinker sepilinker_client{aby_config, epi_config};
    sepilinker_client.build_circuit(nvals);
    sepilinker_client.run_setup_phase();
    EpilinkClientInput client_input{hw_data, bin_data, hw_empty, bin_empty,
                                    nvals};
    fmt::print("Client running\n{}\n{}\n{}\n",
        aby_config, epi_config, client_input);
    const auto client_share{sepilinker_client.run_as_client(client_input)};
    fmt::print("Client result:\n{}\n", client_share);
  } catch (const exception& e) {
    fmt::print(stderr, "Error running MPC Client: {}\n", e.what());
    m_status = JobStatus::FAULT;
  }
  m_status = JobStatus::DONE;
}

void LinkageJob::set_local_config(shared_ptr<LocalConfiguration> l_config) {
  m_local_config = move(l_config);
}
}  // namespace sel
