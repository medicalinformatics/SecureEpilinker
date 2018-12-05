/**
\file    headerhandlerfunctions.cpp
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
\brief Containes functions for the HeaderMethodHander
*/
#include "headerhandlerfunctions.h"
#include <string>
#include <map>
#include <thread>
#include "resttypes.h"
#include "restbed"
#include "restresponses.hpp"
#include "serverhandler.h"
#include "configurationhandler.h"
#include "remoteconfiguration.h"
#include "connectionhandler.h"
#include "logger.h"
#include "util.h"

using namespace std;
namespace sel{

SessionResponse init_mpc(const shared_ptr<restbed::Session>&,
                              const shared_ptr<const restbed::Request>&,
                              const multimap<string,string>& header,
                              string remote_id,
                              const shared_ptr<spdlog::logger>& logger) {
  SessionResponse response;
  Port aby_server_port;
  string client_ip;
  bool counting_mode;
  logger->info("Recieved Linkage Request from {}", remote_id);
  auto remote_config{ConfigurationHandler::cget().get_remote_config(remote_id)};
  if(auto auth_result = // check authentication
      remote_config->get_remote_authenticator().check_authentication_header(header);
      auth_result.return_code != 200){ // auth not ok
    return auth_result;
  }
  if(header.find("Record-Number") == header.end()) {
    logger->error("No client record number from {}", remote_id);
    return responses::status_error(400, "No client record number transmitted");
  }
  if(header.find("Counting-Mode") == header.end()) {
    counting_mode = false;
  }
  aby_server_port = ServerHandler::cget().get_server_port(remote_id);
  size_t num_records = stoull(header.find("Record-Number")->second);
  counting_mode = header.find("Counting-Mode")->second == "true" ? true : false;
  size_t server_record_number;
  shared_ptr<const ServerData> data;
  try {
    server_record_number = DataHandler::get().poll_database(remote_id, counting_mode);
    data = DataHandler::get().get_database();
  } catch (const exception& e){
    logger->error("Error geting data from dataservice: {}", e.what());
    return sel::responses::status_error(restbed::INTERNAL_SERVER_ERROR, "Can not get data from dataservice");
  }
  response.return_code = restbed::OK;
  if(!counting_mode){
    response.body = "Linkage server running"s;
  } else {
    response.body = "Counting server running"s;
  }
  response.headers = {{"Content-Length", to_string(response.body.length())},
                      {"Record-Number", to_string(server_record_number)},
                      {"SEL-Port", to_string(aby_server_port)},
                      {"Connection", "Close"}};
  std::thread server_runner([remote_id, data, num_records, counting_mode]() {
      ServerHandler::get().run_server(remote_id, data, num_records, counting_mode);
  });
  server_runner.detach();
  return response;
}

SessionResponse test_configs(const shared_ptr<restbed::Session>&,
                              const shared_ptr<const restbed::Request>&,
                              const multimap<string,string>& header,
                              const string& remote_id,
                              const shared_ptr<spdlog::logger>& logger) {
  SessionResponse response;
  logger->info("Recieved Test Request from {}", remote_id);
  //if(auto auth_result = // check authentication
      //remote_config->get_remote_authenticator().check_authentication_header(header);
      //auth_result.return_code != 200){ // auth not ok
    //return auth_result;
  //}
  auto& config_handler{ConfigurationHandler::get()};
  config_handler.get_remote_config(remote_id)->test_configuration(config_handler.get_local_config()->get_local_id(), config_handler.make_comparison_config(remote_id));
  response.return_code = restbed::OK;
  response.body = "Remotes connected"s;
  response.headers = {{"Content-Length", to_string(response.body.length())},
                      {"SEL-Identifier", remote_id},
                      {"Connection", "Close"}};
  return response;
}

SessionResponse test_linkage_service(const shared_ptr<restbed::Session>&,
                              const shared_ptr<const restbed::Request>&,
                              const multimap<string,string>&,
                              const string& remote_id,
                              const shared_ptr<spdlog::logger>& logger) {
  SessionResponse response;
  logger->info("Recieved Test Linkage Service Request for {}", remote_id);
  auto remote_config{ConfigurationHandler::cget().get_remote_config(remote_id)};
  //if(auto auth_result = // check authentication
      //remote_config->get_remote_authenticator().check_authentication_header(header);
      //auth_result.return_code != 200){ // auth not ok
    //return auth_result;
  //}
  try{
    remote_config->test_linkage_service();
  } catch(const exception& e) {
      logger->error("Error connecting to Linkage Service: {}", e.what());
      return responses::status_error(500, e.what());
  }
    response.return_code = restbed::OK;
    response.body = "Linkage Service connected"s;
    response.headers = {{"Content-Length", to_string(response.body.length())},
                        {"Connection", "Close"}};
    return response;
}
} // namespace sel
