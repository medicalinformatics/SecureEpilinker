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
#include "serverhandler.h"
#include "configurationhandler.h"
#include "remoteconfiguration.h"
#include "connectionhandler.h"
#include "logger.h"

using namespace std;
namespace sel{

SessionResponse init_mpc(const shared_ptr<restbed::Session>&,
                              const shared_ptr<const restbed::Request>&,
                              const multimap<string,string>&,
                              string remote_id,
                              const shared_ptr<ConfigurationHandler>&,
                              const shared_ptr<ConnectionHandler>&,
                              const shared_ptr<ServerHandler>& server_handler,
                              const shared_ptr<DataHandler>& data_handler,
                              const shared_ptr<spdlog::logger>& logger) {
  SessionResponse response;
  Port common_port;
  string client_ip;
  logger->info("Recieved Linkage Request from {}", remote_id);
  // TODO(TK) Authorization
  common_port = server_handler->get_server_port(remote_id);

  auto nvals{data_handler->poll_database(remote_id)};
  auto data{data_handler->get_database()};
  response.return_code = restbed::OK;
  response.body = "Linkage server running"s;
  response.headers = {{"Content-Length", to_string(response.body.length())},
                      {"SEL-Identifier", remote_id},
                      {"Record-Number", to_string(nvals)},
                      {"SEL-Port", to_string(common_port)},
                      {"Connection", "Close"}};
  std::thread server_runner([remote_id, data, server_handler]() {
    server_handler->run_server(remote_id, data);
  });
  server_runner.detach();
  return response;
}

SessionResponse test_configs(const shared_ptr<restbed::Session>&,
                              const shared_ptr<const restbed::Request>&,
                              const multimap<string,string>&,
                              string remote_id,
                              const shared_ptr<ConfigurationHandler>& config_handler,
                              const shared_ptr<ConnectionHandler>& connection_handler,
                              const shared_ptr<ServerHandler>& server_handler,
                              const shared_ptr<DataHandler>& data_handler,
                              const shared_ptr<spdlog::logger>& logger) {
  SessionResponse response;
  logger->info("Recieved Test Request from {}", remote_id);
  config_handler->get_remote_config(remote_id)->test_configuration(config_handler->get_local_config()->get_local_id(), config_handler->make_comparison_config(remote_id), connection_handler, server_handler);
  response.return_code = restbed::OK;
  response.body = "Linkage server running"s;
  response.headers = {{"Content-Length", to_string(response.body.length())},
                      {"SEL-Identifier", remote_id},
                      {"Connection", "Close"}};
  return response;
}

} // namespace sel
