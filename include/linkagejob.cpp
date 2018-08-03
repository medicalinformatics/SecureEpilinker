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
#include "apikeyconfig.hpp"
#include "authenticationconfig.hpp"
#include "remoteconfiguration.h"
#include "secure_epilinker.h"
#include "util.h"
#include "logger.h"

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

void LinkageJob::set_callback(string&& cc) {
  m_callback = move(cc);
}

void LinkageJob::add_data_field(const FieldName& fieldname,
                                DataField datafield) {
  FieldEntry temp_entry;
  const auto field_info{m_local_config->get_field(fieldname)};
    // FIXME(TK) I do s.th. *very* unsafe and use bitlength user input directly
    // for memcpy. DO SOME SANITY CHECKS OR THIS SOFTWARE WILL BREAK AND ALLOW
    // ARBITRARY REMOTE CODE EXECUTION!
  if(holds_alternative<nullptr_t>(datafield)) {
    temp_entry = nullopt;
  } else if (holds_alternative<int>(datafield)) {
    const auto content{get<int>(datafield)};
      Bitmask temp(bitbytes(field_info.bitsize));
      ::memcpy(temp.data(), &content, bitbytes(field_info.bitsize));
      temp_entry = move(temp);
  } else if (holds_alternative<double>(datafield)) {
    const auto content{get<double>(datafield)};
      Bitmask temp(bitbytes(field_info.bitsize));
      ::memcpy(temp.data(), &content, bitbytes(field_info.bitsize));
      temp_entry = move(temp);
  } else if (holds_alternative<string>(datafield)) {
    const auto content{get<string>(datafield)};
      const auto temp_char_array{content.c_str()};
      Bitmask temp(bitbytes(field_info.bitsize));
      ::memcpy(temp.data(), temp_char_array, bitbytes(field_info.bitsize));
      temp_entry = move(temp);
  } else if (holds_alternative<Bitmask>(datafield)) {
    const auto content{get<Bitmask>(datafield)};
      temp_entry = move(content);
  }
  m_data.emplace(fieldname, temp_entry);
}

JobStatus LinkageJob::get_status() const {
  return m_status;
}

JobId LinkageJob::get_id() const {
  return m_id;
}

void LinkageJob::run_linkage_job() {
  auto logger{get_default_logger()};
  m_status = JobStatus::RUNNING;

  // Prepare Data, weights and exchange groups

  logger->info("Job {} started\n", m_id);
#ifdef DEBUG_SEL_REST
  auto debugger = m_parent->get_data_handler()->get_epilink_debug();
  debugger->reset();
#endif

  try {
    // Get number of records from server
    promise<size_t> nvals_prom;
    auto nvals{nvals_prom.get_future()};
    signal_server(nvals_prom);
    nvals.wait_for(15s);
    if(!nvals.valid()){
      throw runtime_error("Error retrieving number of records from server");
    }
    const auto nvals_value{nvals.get()};
    logger->debug("Server has {} Records\n", nvals_value);
    auto epilinker{m_parent->get_epilink_client(m_remote_config->get_id())};
    epilinker->build_circuit(nvals_value);
    epilinker->run_setup_phase();
    EpilinkClientInput client_input{m_data, nvals_value};
    // run mpc
    const auto client_share{epilinker->run_as_client(client_input)};
    // reset epilinker for the next linkage
    epilinker->reset();
#ifdef DEBUG_SEL_REST
    print_data();
#endif
    logger->info("Client Result: {}", client_share);
    nlohmann::json result{{"idType", "srl1"},{"idString", "3lk4j3Y4l5j"}};
    if(!m_remote_config->get_matching_mode()){
      //TODO(tk) send result to linkage server
    } else {
      #ifdef SEL_MATCHING_MODE
      result = {{"match", true}, {"tmatch", true}};
      #endif
    }
    perform_callback(result);
#ifdef DEBUG_SEL_REST
    debugger->client_input = make_shared<EpilinkClientInput>(client_input);
    if(!(debugger->epilink_config)) {
      debugger->epilink_config = make_shared<EpilinkConfig>(make_epilink_config(m_local_config, m_algo_config, m_remote_config->get_matching_mode()));
    }
    //debugger->epilink_config->set_precisions(5,11);
    logger->debug("Clear Precision: Dice {},\tWeight {}", debugger->epilink_config->dice_prec,debugger->epilink_config->weight_prec);
    if(debugger->all_values_set()){
      if(!debugger->run) {
      fmt::print("============= Integer Computation ============\n");
      debugger->compute_int();
      logger->info("Integer Result: {}", debugger->int_result);
      fmt::print("============= Double Computation =============\n");
      debugger->compute_double();
      logger->info("Double Result: {}", debugger->double_result);
      debugger->run=true;
      }
    } else {
      string ss{debugger->server_input?"Set":"Not Set"};
      string cs{debugger->client_input?"Set":"Not Set"};
      string ec{debugger->epilink_config?"Set":"Not Set"};
      logger->warn("Server: {}, Client: {}, Config: {}\n", ss, cs, ec);
    }
#endif
    m_status = JobStatus::DONE;
  } catch (const exception& e) {
    logger->error("Error running MPC Client: {}\n", e.what());
    m_status = JobStatus::FAULT;
  }
}

void LinkageJob::run_matching_job() {
  auto logger{get_default_logger()};
  logger->warn("A matching job is starting.");
  run_linkage_job();
}

void LinkageJob::set_local_config(shared_ptr<LocalConfiguration> l_config) {
  m_local_config = move(l_config);
}

/**
 * Send server the configuration to compare and recieve back the number of
 * records in the database
 */
void LinkageJob::signal_server(promise<size_t>& nvals) {
  auto logger{get_default_logger()};
  std::this_thread::sleep_for(1s);
  curlpp::Easy curl_request;
  // Create configurations to com,pare
  nlohmann::json algo_comp_conf{*m_algo_config};
  algo_comp_conf.emplace_back(m_local_config->get_comparison_json());
  auto data{algo_comp_conf.dump()};
  // Create http request
  promise<stringstream> response_promise;
  future<stringstream> response_stream = response_promise.get_future();
  list<string> headers{
      "Authorization: ",
      "SEL-Identifier: "s + m_local_config->get_local_id(),
      "Expect:",
      "Content-Type: application/json",
      "Content-Length: "s+to_string(algo_comp_conf.dump().size())};
  curl_request.setOpt(new curlpp::Options::HttpHeader(headers));
  curl_request.setOpt(new curlpp::Options::Url(
      "https://"s + m_remote_config->get_remote_host() + ':' +
      to_string(m_remote_config->get_remote_signaling_port()) + "/initMPC/"+m_local_config->get_local_id()));
  curl_request.setOpt(new curlpp::Options::Post(true));
  curl_request.setOpt(new curlpp::Options::Verbose(false));
  curl_request.setOpt(new curlpp::Options::SslVerifyHost(false));
  curl_request.setOpt(new curlpp::Options::SslVerifyPeer(false));
  curl_request.setOpt(new curlpp::Options::PostFields(data.c_str()));
  curl_request.setOpt(new curlpp::Options::PostFieldSize(algo_comp_conf.dump().size()));
  curl_request.setOpt(new curlpp::options::Header(1));
  logger->debug("Sending linkage request\n");
  try{
    send_curl(curl_request, move(response_promise));
    response_stream.wait();
    auto stream = response_stream.get();
    logger->debug("Response stream:\n{}\n", stream.str());
    // get nvals from response header
    nvals.set_value(stoull(get_headers(stream, "Record-Number").front()));
  } catch (const exception& e) {
    logger->error("Error performing initMPC call: {}", e.what());
  }
}

bool LinkageJob::perform_callback(const nlohmann::json& new_id) const {
  auto logger{get_default_logger()};
  curlpp::Easy curl_request;
  nlohmann::json data_json{"newId", {
                          {"idType", new_id["idType"].get<string>()},
                          {"idString", new_id["idString"].get<string>()} }
                        };
  auto data{data_json.dump()};
  promise<stringstream> response_promise;
  future<stringstream> response_stream{response_promise.get_future()};
  const auto auth{m_local_config->get_local_authentication()};
  auto local_auth =  dynamic_cast<const APIKeyConfig*>(auth);
  list<string> headers{
      "Authorization: "s+local_auth->get_key(),
      "Expect:",
      "Content-Type: application/json",
      "Content-Length: "s+to_string(data.length())};
  curl_request.setOpt(new curlpp::Options::HttpHeader(headers));
  curl_request.setOpt(new curlpp::Options::Url(m_callback));
  curl_request.setOpt(new cURLpp::Options::Verbose(false));
  curl_request.setOpt(new curlpp::Options::Post(true));
  curl_request.setOpt(new curlpp::Options::PostFields(data.c_str()));
  curl_request.setOpt(new curlpp::Options::PostFieldSize(data.size()));
  curl_request.setOpt(new curlpp::Options::SslVerifyHost(false));
  curl_request.setOpt(new curlpp::Options::SslVerifyPeer(false));
  curl_request.setOpt(new curlpp::options::Header(1));
  logger->debug("Sending callback to: {}\n", m_callback);
  send_curl(curl_request, move(response_promise));
  response_stream.wait();
  auto stream = response_stream.get();
  logger->trace("Callback response:\n{}\n", stream.str());
  return true; //TODO(TK) return correct success bool
}

#ifdef DEBUG_SEL_REST
void LinkageJob::print_data() const {
  auto logger{get_default_logger()};
  string input_string;
  for (auto& p : m_data) {
    input_string += "-------------------------------\n" + p.first +
                    "\n-------------------------------"
                    "\n";
    const auto& d = p.second;
    bool empty{!d};
    input_string += "Field "s + (empty ? "" : "not ") + "empty ";
    if (!empty) {
      for (const auto& byte : d.value())
        input_string += to_string(byte) + " ";
    }
    input_string += "\n";
  }
  logger->trace(input_string);
}
#endif
}  // namespace sel
