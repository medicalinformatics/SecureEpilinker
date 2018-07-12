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
#include "resttypes.h"
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
#include <future>

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

void LinkageJob::set_callback(CallbackConfig cc) {
  m_callback = move(cc);
}

void LinkageJob::add_data_field(const FieldName& fieldname,
                                DataField datafield) {
  FieldEntry temp_entry;
  const auto field_info{m_local_config->get_field(fieldname)};
    // FIXME(TK) I do s.th. *very* unsafe and use bitlength user input directly
    // for memcpy. DO SOME SANITY CHECKS OR THIS SOFTWARE WILL BREAK AND ALLOW
    // ARBITRARY REMOTE CODE EXECUTION!
  if (holds_alternative<int>(datafield)) {
    const auto content{get<int>(datafield)};
    if (content == 0) {
      temp_entry = nullopt;
    } else {
      Bitmask temp(bitbytes(field_info.bitsize));
      ::memcpy(temp.data(), &content, bitbytes(field_info.bitsize));
      temp_entry = move(temp);
    }
  } else if (holds_alternative<double>(datafield)) {
    const auto content{get<double>(datafield)};
    if (content == 0.) {
      temp_entry = nullopt;
    } else {
      Bitmask temp(bitbytes(field_info.bitsize));
      ::memcpy(temp.data(), &content, bitbytes(field_info.bitsize));
      temp_entry = move(temp);
    }
  } else if (holds_alternative<string>(datafield)) {
    const auto content{get<string>(datafield)};
    if (trim_copy(content).empty()) {
      temp_entry = nullopt;
    } else {
      const auto temp_char_array{content.c_str()};
      Bitmask temp(bitbytes(field_info.bitsize));
      ::memcpy(temp.data(), temp_char_array, bitbytes(field_info.bitsize));
      temp_entry = move(temp);
    }
  } else if (holds_alternative<Bitmask>(datafield)) {
    const auto content{get<Bitmask>(datafield)};
    bool bloomempty{true};
    for (const auto& byte : content) {
      if (bool byte_empty = (byte == 0x00); !byte_empty) {
        bloomempty = false;
        break;
      }
    }
    if (bloomempty) {
      temp_entry = nullopt;
    } else {
      temp_entry = move(content);
    }
  }
  m_data.emplace(fieldname, temp_entry);
}

JobStatus LinkageJob::get_status() const {
  return m_status;
}

JobId LinkageJob::get_id() const {
  return m_id;
}

void LinkageJob::run_job() {
  m_status = JobStatus::RUNNING;

  // Prepare Data, weights and exchange groups

  fmt::print("Job {} started\n", m_id);

  try {
    // Get number of records from server
    size_t nvals = signal_server();
    fmt::print("Server has {} Records\n", nvals);
    auto epilinker{m_parent->get_epilink_client(m_remote_config->get_id())};
    epilinker->build_circuit(nvals);
    epilinker->run_setup_phase();
    EpilinkClientInput client_input{m_data, nvals};
    const auto client_share{epilinker->run_as_client(client_input)};
    fmt::print("Client result:\nIndex: {}, Match: {}, TMatch:{}\n", client_share.index, client_share.match, client_share.tmatch);
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
  nlohmann::json algo_comp_conf{*m_algo_config};
  algo_comp_conf.emplace_back(m_local_config->get_comparison_json());
  auto data{algo_comp_conf.dump()};
  promise<stringstream> response_promise;
  future<stringstream> response_stream = response_promise.get_future();
  list<string> headers{
      "Authorization: SEL ABCD",
      "Remote-Identifier: "s + m_remote_config->get_remote_client_id(),
      "Expect:",
      "Content-Type: text/json",
      "Content-Length: "s+to_string(algo_comp_conf.dump().size())};
  curl_request.setOpt(new curlpp::Options::HttpHeader(headers));
  curl_request.setOpt(new curlpp::Options::Url(
      "http://"s + m_remote_config->get_remote_host() + ':' +
      to_string(m_remote_config->get_remote_signaling_port()) + "/sellink/"+m_remote_config->get_remote_client_id()));
  curl_request.setOpt(new curlpp::Options::Post(true));
  curl_request.setOpt(new curlpp::Options::SslVerifyHost(false));
  curl_request.setOpt(new curlpp::Options::SslVerifyPeer(false));
  curl_request.setOpt(new curlpp::Options::PostFields(data.c_str()));
  curl_request.setOpt(new curlpp::Options::PostFieldSize(algo_comp_conf.dump().size()));
  curl_request.setOpt(new curlpp::options::Header(1));
  fmt::print("Sending linkage request\n");
  send_curl(curl_request, move(response_promise));
  response_stream.wait();
  auto stream = response_stream.get();
  fmt::print("Response stream: {}\n", stream.str());
  auto nval{stoull(get_headers(stream, "Record-Number").front())};
  return nval;
}
}  // namespace sel
