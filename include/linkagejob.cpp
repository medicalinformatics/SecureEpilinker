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
#include "restutils.h"
#include "serverhandler.h"

#include "epilink_input.h"
#include "apikeyconfig.hpp"
#include "authenticationconfig.hpp"
#include "remoteconfiguration.h"
#include "secure_epilinker.h"
#include "util.h"
#include "logger.h"

#include <future>

using namespace std;
namespace sel {

LinkageJob::LinkageJob() : m_id(generate_id()) {}

LinkageJob::LinkageJob(shared_ptr<const LocalConfiguration> l_conf,
                       shared_ptr<const RemoteConfiguration> r_conf)
    : m_id(generate_id()),
      m_local_config(move(l_conf)),
      m_remote_config(move(r_conf)) {}

void LinkageJob::set_callback(string&& cc) {
  m_callback = move(cc);
}

void LinkageJob::add_data_field(const FieldName& fieldname,
                                FieldEntry datafield) {
  m_data.emplace(fieldname, move(datafield));
}

void LinkageJob::add_data(map<FieldName, FieldEntry> data) {
  m_data = move(data);
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
  auto debugger = DataHandler::get().get_epilink_debug();
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
    auto epilinker{ServerHandler::get().get_epilink_client(m_remote_config->get_id())};
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
    if(!m_remote_config->get_matching_mode()){
      auto response{send_result_to_linkageservice(client_share, "client", m_local_config, m_remote_config)};
      if (response.return_code == 200) {
        perform_callback(response.body);
      }
    } else {
      #ifdef SEL_MATCHING_MODE
      nlohmann::json match_json;
      nlohmann::json match_result;
      match_result["match"] = client_share.match;
      match_result["tentativeMatch"] = client_share.tmatch;
      match_json["result"] = match_result;
      logger->trace("Result to callback: {}", match_json.dump(0));
      perform_callback(match_json.dump());
      #endif
    }
#ifdef DEBUG_SEL_REST
    debugger->client_input = make_shared<EpilinkClientInput>(client_input);
    if(!(debugger->circuit_config)) {
      debugger->circuit_config = make_shared<CircuitConfig>(make_circuit_config(m_local_config, m_remote_config));
    }
    //debugger->epilink_config->set_precisions(5,11);
    logger->debug("Clear Precision: Dice {},\tWeight {}", debugger->circuit_config->dice_prec,debugger->circuit_config->weight_prec);
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
      string ec{debugger->circuit_config?"Set":"Not Set"};
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
  // TODO(TK): get inter SEL Authorization stuff done
  string data{"{}"};
  list<string> headers{
      "Authorization: apiKey apiKey=\""s+"\"",
      "SEL-Identifier: "s + m_local_config->get_local_id(),
      "Content-Type: application/json"};
  string url{assemble_remote_url(m_remote_config) + "/initMPC/"+m_local_config->get_local_id()};
  logger->debug("Sending linkage request to {}\n", url);
  try{
    auto response{perform_post_request(url, data, headers, true)};
    logger->debug("Response stream:\n{} - {}\n",response.return_code, response.body);
    // get nvals from response header
    if (response.return_code == 200) {
      nvals.set_value(stoull(get_headers(response.body, "Record-Number").front()));
    } else {
      logger->error("Error communicating with remote epilinker: {} - {}", response.return_code, response.body);
    }
  } catch (const exception& e) {
    logger->error("Error performing initMPC call: {}", e.what());
  }
}

bool LinkageJob::perform_callback(const string& body) const {
  auto logger{get_default_logger()};
  const auto auth{m_local_config->get_local_authentication()};
  auto local_auth =  dynamic_cast<const APIKeyConfig*>(auth);
  list<string> headers{
      "Authorization: apiKey apiKey=\""s+local_auth->get_key()+"\"",
      "Content-Type: application/json"};
  logger->debug("Sending callback to: {}\n", m_callback);
  auto response{perform_post_request(m_callback, body, headers, true)};
  logger->trace("Callback response:\n{} - {}\n", response.return_code, response.body);
  return response.return_code == 200;
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
