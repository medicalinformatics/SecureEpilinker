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
#include "util.h"
#include "logger.h"
#include "seltypes.h"
#include "resttypes.h"
#include "restutils.h"
#include "localconfiguration.h"
#include "serverhandler.h"
#include "epilink_input.h"
#include "secure_epilinker.h"
#include "apikeyconfig.hpp"
#include "authenticationconfig.hpp"
#include "remoteconfiguration.h"
#include "fmt/format.h"
#include <exception>
#include <map>
#include <string>
#include <variant>
#include <vector>
#include <optional>
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

void LinkageJob::add_data(unique_ptr<Records> data) {
  m_records = move(data);
}

JobStatus LinkageJob::get_status() const {
  return m_status;
}

void LinkageJob::set_status(JobStatus status){
  m_status = status;
}

JobId LinkageJob::get_id() const {
  return m_id;
}

RemoteId LinkageJob::get_remote_id() const {
  return m_remote_config->get_id();
}

LinkageJob::JobPreparation LinkageJob::prepare_run() {
  m_status = JobStatus::RUNNING;
  // Get number of records from server
  promise<size_t> nvals_prom;
  auto nvals{nvals_prom.get_future()};
  size_t num_records{m_records->size()};
  signal_server(nvals_prom, num_records);
  nvals.wait_for(15s);
  if(!nvals.valid()){
    throw runtime_error("Error retrieving number of records from server");
  }
  const auto database_size{nvals.get()};
  return {num_records, database_size, ServerHandler::get().get_epilink_client(m_remote_config->get_id())};
}


void LinkageJob::run_linkage_job() {
  auto logger{get_default_logger()};
  logger->info("Linkage job {} started\n", m_id);
  try {
    auto [num_records, database_size, epilinker] = prepare_run();
    logger->debug("Client has {} Records\n", num_records);
    logger->debug("Server has {} Records\n", database_size);
    epilinker->build_linkage_circuit(num_records, database_size);
    epilinker->run_setup_phase();
#ifdef DEBUG_SEL_REST
      print_data();
#endif
    epilinker->set_client_input({move(m_records), database_size});
    auto linkage_share{epilinker->run_linkage()};
      // reset epilinker for the next linkage
      epilinker->reset();
      logger->info("Client Result: {}", linkage_share);
      try{
        auto response{send_result_to_linkageservice(linkage_share, nullopt , "client", m_local_config, m_remote_config)};
        if (response.return_code == 200) {
          perform_callback(response.body);
        }
      } catch (const exception& e) {
        logger->error("Can not connect to linkage service or callback: {}", e.what());
      }
    m_status = JobStatus::DONE;
  } catch (const exception& e) {
    logger->error("Error running MPC Client: {}\n", e.what());
    m_status = JobStatus::FAULT;
  }
}

void LinkageJob::run_matching_job() {
  auto logger{get_default_logger()};
#ifdef SEL_MATCHING_MODE
  logger->warn("A matching job is starting.");
  try {
    auto [num_records, database_size, epilinker] = prepare_run();
    logger->debug("Client has {} Records\n", num_records);
    logger->debug("Server has {} Records\n", database_size);
    epilinker->build_count_circuit(num_records, database_size);
    epilinker->run_setup_phase();
#ifdef DEBUG_SEL_REST
      print_data();
#endif
    epilinker->set_input({move(m_records), database_size});
    auto count_result{epilinker->run_count()};
      // reset epilinker for the next operation
      epilinker->reset();
      // The strange assembly of the json is due to strange object/array
      // distinctions in nlohmann/json
      nlohmann::json match_json;
      nlohmann::json match_result;
      match_result["matches"] = count_result.matches;
      match_result["tentativeMatches"] = count_result.tmatches;
      match_json["result"] = match_result;
      logger->trace("Result to callback: {}", match_json.dump(0));
      perform_callback(match_json.dump());
    m_status = JobStatus::DONE;
  } catch (const exception& e) {
    logger->error("Error running MPC Client: {}\n", e.what());
    m_status = JobStatus::FAULT;
  }
#endif
#ifndef SEL_MATCHING_MODE
  logger->error("Matching mode not allowed");
  m_status = JobStatus::FAULT;
#endif
}

void LinkageJob::set_local_config(shared_ptr<LocalConfiguration> l_config) {
  m_local_config = move(l_config);
}

/**
 * Send server the configuration to compare and recieve back the number of
 * records in the database
 */
void LinkageJob::signal_server(promise<size_t>& nvals, size_t num_records) {
  auto logger{get_default_logger()};
  //FIXME(TK): THIS IS BAD AND I SHOULD FEEL BAD
  std::this_thread::sleep_for(1s);
  list<string> headers{
      "Authorization: "s+m_remote_config->get_remote_authenticator().sign_transaction(""),
      "Record-Number: "s + to_string(num_records),
      "Counting-Mode: "s + (m_counting_job ? "true" : "false"),
      "Content-Type: application/json"};
  string url{assemble_remote_url(m_remote_config) + "/initMPC/"+m_local_config->get_local_id()};
  logger->debug("Sending {} request to {}\n",(m_counting_job ? "matching" : "linkage"), url);
  try{
    // TODO(TK): Refactor perform_post_request w/ optional to avoid dummy data
    auto response{perform_post_request(url, "{}", headers, true)};
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
  for (auto& record : *m_records) {
    input_string += "=================================\n";
    for(auto& p : record){ // Every field in Record
      input_string += "-------- " + p.first + " --------\n";
      bool empty{!p.second};
      input_string += (empty ? "Field empty" : "");
      if (!empty) {
        for (const auto& byte : p.second.value()) {
          input_string += to_string(byte) + " ";
        }
      }
      input_string += "\n";
    }
  }
  logger->trace("Client Data:\n{}",input_string);
}
}
#endif
}  // namespace sel
