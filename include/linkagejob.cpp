/**
\file    linkagejob.cpp
\author  Tobias Kussel <kussel@cbs.tu-darmstadt.de>
\copyright SEL - Secure EpiLinker
    Copyright (C) 2018 Computational Biology & Simulation Group TU-Darmstadt
This program is free software: you can redistribute it and/or modify it under
the terms of the GNU Affero General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option) any
later version. This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Affero General Public License for more details.
    You should have received a copy of the GNU Affero General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
\brief Holds information and data for one linkage job
*/

#include "linkagejob.h"
#include <exception>
#include <map>
#include <string>
#include <variant>
#include <vector>
#include "fmt/format.h"
#include "localconfiguration.h"
#include "seltypes.h"
#include "serverhandler.h"

#include "epilink_input.h"
#include "remoteconfiguration.h"
#include "secure_epilinker.h"
#include "util.h"

#include <curlpp/Easy.hpp>
#include <curlpp/Infos.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>
#include <iostream>
#include <sstream>

using namespace std;
namespace sel {

LinkageJob::LinkageJob() : m_id(generate_id()) {}

LinkageJob::LinkageJob(shared_ptr<const LocalConfiguration> l_conf,
                       shared_ptr<const RemoteConfiguration> r_conf,
                       shared_ptr<const AlgorithmConfig> algo,
                       shared_ptr<ServerHandler> server_handler)
    : m_id(generate_id()),
      m_local_config(move(l_conf)),
      m_algo_config(move(algo)),
      m_remote_config(move(r_conf)),
      m_parent(move(server_handler)) {}

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
  VWeight bin_weights{m_local_config->get_weights(FieldComparator::BINARY)};

  // TODO(TK): To get: remote_ip, remote_port, nvals,
  // threads

  try {
    // Construct ABY Client
    // FIXME(TK): Magicnumbers
    size_t nvals = signal_server();
    fmt::print("Server has {} Records\n", nvals);
    auto epilinker{m_parent->get_epilink_client(m_remote_config->get_id())};
    // TODO(TK): Correct position for building circuit (again)?
    epilinker->build_circuit(nvals);
    epilinker->run_setup_phase();
    EpilinkClientInput client_input{hw_data, bin_data, hw_empty, bin_empty,
                                    nvals};
    fmt::print("Client running\n{}", print_epilink_input(client_input));
    // const auto client_share{epilinker->run_as_client(client_input)};
    // fmt::print("Client result:\n{}", client_share);
    fmt::print("Client exited successfuly with dummy values\n");
    m_status = JobStatus::DONE;
  } catch (const exception& e) {
    fmt::print(stderr, "Error running MPC Client: {}\n", e.what());
    m_status = JobStatus::FAULT;
  }
}

void LinkageJob::set_local_config(shared_ptr<LocalConfiguration> l_config) {
  m_local_config = move(l_config);
}

size_t LinkageJob::signal_server() {
  curlpp::Easy curl_request;
  stringstream response_stream;
  list<string> headers{
      "Authorization: SEL ABCD",
      "Remote-Identifier: "s + m_remote_config->get_remote_client_id()};
  curl_request.setOpt(new curlpp::Options::HttpHeader(headers));
  curl_request.setOpt(new curlpp::Options::Url(
      "https://"s + m_remote_config->get_remote_host() + ':' +
      to_string(m_remote_config->get_remote_signaling_port()) + "/selconnect"));
  curl_request.setOpt(new curlpp::Options::Post(true));
  curl_request.setOpt(new curlpp::Options::SslVerifyHost(false));
  curl_request.setOpt(new curlpp::Options::SslVerifyPeer(false));
  curl_request.setOpt(new curlpp::Options::WriteStream(&response_stream));
  curl_request.setOpt(new curlpp::options::Header(1));
  fmt::print("Sending linkage request\n");
  curl_request.perform();
  auto nval{stoull(get_headers(response_stream, "Record-Number").front())};
  return nval;
}

}  // namespace sel
