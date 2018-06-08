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
#include <vector>
#include <variant>
#include "fmt/format.h"
#include "localconfiguration.h"
#include "seltypes.h"

#include "epilink_input.h"
#include "secure_epilinker.h"
#include "remoteconfiguration.h"

#include <iostream>

void sel::LinkageJob::set_id() {
  const auto timestamp{std::chrono::system_clock::now().time_since_epoch()};
  m_id = std::to_string(
      std::chrono::duration_cast<std::chrono::milliseconds>(timestamp).count());
  std::random_shuffle(m_id.begin(), m_id.end());
}

sel::LinkageJob::LinkageJob() {
  set_id();
}

sel::LinkageJob::LinkageJob(std::shared_ptr<sel::LocalConfiguration> l_conf, std::shared_ptr<sel::RemoteConfiguration> parent)
    : m_local_config(std::move(l_conf)), m_parent(parent) {
  set_id();
}

void sel::LinkageJob::add_hw_data_field(const sel::FieldName& fieldname, sel::DataField datafield, bool empty){
 assert(std::holds_alternative<Bitmask>(datafield));
 m_hw_data.emplace(fieldname, std::get<Bitmask>(datafield));
 m_hw_empty.emplace(fieldname, empty);
}

void sel::LinkageJob::add_bin_data_field(const sel::FieldName& fieldname, sel::DataField datafield, bool empty){
 bin_type tempfield;
 if(std::holds_alternative<int>(datafield)){
  tempfield = static_cast<bin_type>(std::get<int>(datafield));
 } else if (std::holds_alternative<double>(datafield)){
  tempfield = std::hash<double>{}(std::get<double>(datafield));
 } else if(std::holds_alternative<std::string>(datafield)){
   tempfield = std::hash<std::string>{}(std::get<std::string>(datafield));
 }else {
  throw std::runtime_error("Invalid Binary Comparison");
  tempfield = 0;
 }
 m_bin_empty.emplace(fieldname, empty);
 m_bin_data.emplace(fieldname, tempfield);
}

void sel::LinkageJob::run_job() {
  m_status = JobStatus::RUNNING;

  // Prepare Data, weights and exchange groups
  
  const auto& algorithm_config{m_local_config->get_algorithm_config()};

  std::vector<sel::Bitmask> hw_data;
  sel::v_bin_type bin_data;
  hw_data.reserve(m_hw_data.size());
  bin_data.reserve(m_bin_data.size());

  std::vector<bool> hw_empty;
  std::vector<bool> bin_empty;
  hw_empty.reserve(m_hw_empty.size());
  bin_empty.reserve(m_bin_empty.size());

  for (const auto& field : m_hw_data){
    hw_data.emplace_back(field.second);
  }
  for (const auto& field : m_bin_data){
    bin_data.emplace_back(field.second);
  }
  for (const auto& field : m_hw_empty){
    hw_empty.emplace_back(field.second);
  }
  for (const auto& field : m_bin_empty){
    bin_empty.emplace_back(field.second);
  }

  sel::VWeight hw_weights{m_local_config->get_weights(sel::FieldComparator::NGRAM)};
  sel::VWeight bin_weights{m_local_config->get_weights(sel::FieldComparator::BINARY)};
  
  // TODO(TK): To get: remote_ip, remote_port, nvals,
  // threads

  try {
    // Construct ABY Client
    // FIXME(TK): Magicnumbers
    const e_sharing booleantype{S_BOOL};
    const uint32_t nthreads{1};
    size_t nvals;
    fmt::print("Please insert number of records (no worries, this is only temporary)\n");
    std::cin >> nvals;
    sel::SecureEpilinker sepilinker_client{
        {CLIENT, booleantype, m_parent->get_remote_host(), m_parent->get_remote_port(), nthreads},
        {hw_weights, bin_weights, m_local_config->get_exchange_group_indices(sel::FieldComparator::NGRAM), m_local_config->get_exchange_group_indices(sel::FieldComparator::BINARY),
         algorithm_config.bloom_length, algorithm_config.threshold_match, algorithm_config.threshold_non_match}};
    sepilinker_client.build_circuit(nvals);
    sepilinker_client.run_setup_phase();
    sel::EpilinkClientInput client_input{hw_data, bin_data, hw_empty, bin_empty, nvals};
    fmt::print("Client Running\n");
    //sepilinker_client.run_as_client(client_input);
  } catch (const std::exception& e) {
    fmt::print(stderr, "Error running MPC Client: {}\n", e.what());
    m_status = JobStatus::FAULT;
  }
  m_status = JobStatus::DONE;
}

void sel::LinkageJob::set_local_config(
    std::shared_ptr<sel::LocalConfiguration> l_config) {
  m_local_config = std::move(l_config);
}
