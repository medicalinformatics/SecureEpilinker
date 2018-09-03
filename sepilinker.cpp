/**
\file    epirest.cpp
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
\brief Main entry point for the rest interface of the secure EpiLinker
*/

#include "include/remoteconfiguration.h"
#include "include/apikeyconfig.hpp"
#include "include/connectionhandler.h"
#include "include/configurationhandler.h"
#include "include/datahandler.h"
#include "include/serverhandler.h"
#include "include/jsonmethodhandler.h"
#include "include/methodhandler.hpp"
#include "include/monitormethodhandler.h"
#include "include/resourcehandler.h"
#include "include/restutils.h"
#include "include/jsonutils.h"
#include "include/validator.h"
#include "include/jsonhandlerfunctions.h"
#include "include/headermethodhandler.h"
#include "include/headerhandlerfunctions.h"

#include "fmt/format.h"
#include "nlohmann/json.hpp"
#include "cxxopts.hpp"
#include "restbed"

#include <functional>
#include <memory>
#include <string>
#include <curlpp/cURLpp.hpp>

#include "include/logger.h"
#include <spdlog/spdlog.h>

#include "include/epilink_input.h"

using json = nlohmann::json;
using namespace sel;

int main(int argc, char* argv[]) {
  // Commandline Parser
  cxxopts::Options options("Secure EpiLinker", "Secure Multi Party Recard Linkage via EpiLink algorithm, Version 0.1.0");
  options.add_options()
    ("c,config", "Config file name", cxxopts::value<std::string>()->default_value("../data/serverconf.json"))
    ("i,localschema", "File name of local initialization schema", cxxopts::value<std::string>())
    ("I,remoteschema", "File name of remote initialization schema", cxxopts::value<std::string>())
    ("l,linkschema", "File name of linkRecord schema", cxxopts::value<std::string>())
    ("L,logfile", "File name of log file", cxxopts::value<std::string>())
    ("k,key", "File name of server key", cxxopts::value<std::string>())
    ("v,verbose", "Log more information")
    ("d,dh", "File name of Diffi-Hellman group", cxxopts::value<std::string>())
    ("C,cert", "File name of server certificate", cxxopts::value<std::string>())
    ("p,port", "Port for listening", cxxopts::value<unsigned>())
    ("s,ssl", "Use SSL")
    ("h,help", " Print this help");
  auto cmdoptions{options.parse(argc,argv)};
  if(cmdoptions.count("help")) {
    fmt::print("{}\n",options.help());
    return 0;
  }
  auto server_config = read_json_from_disk(cmdoptions["config"].as<std::string>());

  // Override config file
  if(cmdoptions.count("localschema")){
    server_config["localInitSchemaPath"] = cmdoptions["localschema"].as<std::string>();
  }
  if(cmdoptions.count("remoteschema")){
    server_config["remoteInitSchemaPath"] = cmdoptions["remoteschema"].as<std::string>();
  }
  if(cmdoptions.count("linkschema")){
    server_config["linkRecordSchemaPath"] = cmdoptions["linkschema"].as<std::string>();
  }
  if(cmdoptions.count("key")){
    server_config["serverKeyPath"] = cmdoptions["key"].as<std::string>();
  }
  if(cmdoptions.count("dh")){
    server_config["serverDHPath"] = cmdoptions["dh"].as<std::string>();
  }
  if(cmdoptions.count("cert")){
    server_config["serverCertificatePath"] = cmdoptions["cert"].as<std::string>();
  }
  if(cmdoptions.count("port")){
    server_config["port"] = cmdoptions["port"].as<unsigned>();
  }
  if(cmdoptions.count("logfile")){
    server_config["logFilePath"] = cmdoptions["logfile"].as<std::string>();
  }
  if(cmdoptions.count("ssl")){
    server_config["useSSL"] = true;
  }

  create_file_logger(server_config.at("logFilePath").get<std::string>());
  switch(cmdoptions.count("verbose")){
    case 0: spdlog::set_level(spdlog::level::warn); break;
    case 1: spdlog::set_level(spdlog::level::info); break;
    case 2: spdlog::set_level(spdlog::level::debug); break;
    default: spdlog::set_level(spdlog::level::trace); break;
  }
    // Program
  auto logger = get_default_logger();

  restbed::Service service;
  curlpp::Cleanup curl_cleanup;
  // Create Connection Handler
  auto& connections = sel::ConnectionHandler::get();
  connections.set_service(&service);
  auto& configurations = sel::ConfigurationHandler::get();
  sel::DataHandler::get(); // instantiate singletons
  sel::ServerHandler::get(); // instantiate singletons

  configurations.set_server_config(parse_json_server_config(server_config));
  try{
  test_server_config_paths(configurations.get_server_config());
  } catch (const std::exception& e) {
    logger->critical("Can not create server configuration: {}", e.what());
    exit(1);
  }
  connections.populate_aby_ports();

  // Create JSON Validator
  auto restconf{configurations.get_server_config()};
  auto init_local_validator = std::make_shared<sel::Validator>(
      read_json_from_disk(restconf.local_init_schema_file));
  auto init_remote_validator = std::make_shared<sel::Validator>(
      read_json_from_disk(restconf.remote_init_schema_file));
  auto linkrecord_validator =
      std::make_shared<sel::Validator>(read_json_from_disk(
          restconf.link_record_schema_file));
  auto null_validator = std::make_shared<sel::Validator>();
  // Create Handlers for INIT Phase
  auto init_local_methodhandler =
      sel::MethodHandler::create_methodhandler<sel::JsonMethodHandler>(
          "PUT", init_local_validator,
          sel::valid_init_local_json_handler, sel::invalid_json_handler);
  auto init_remote_methodhandler =
      sel::MethodHandler::create_methodhandler<sel::JsonMethodHandler>(
          "PUT", init_remote_validator,
          sel::valid_init_remote_json_handler, sel::invalid_json_handler);
  // Create Handlers for Record Linkage phase
  auto linkrecord_methodhandler =
      sel::MethodHandler::create_methodhandler<sel::JsonMethodHandler>(
          "POST", linkrecord_validator,
          sel::valid_linkrecord_json_handler, sel::invalid_json_handler);
  auto matchrecord_methodhandler =
      sel::MethodHandler::create_methodhandler<sel::JsonMethodHandler>(
          "POST", linkrecord_validator,
          sel::valid_linkrecord_json_handler, sel::invalid_json_handler);
  // Create GET-Handler for job status monitoring
  auto jobmonitor_methodhandler =
      sel::MethodHandler::create_methodhandler<sel::MonitorMethodHandler>(
          "GET");
  // Create Handlers for temporary inter SEL communication
  auto test_config_methodhandler =
      sel::MethodHandler::create_methodhandler<sel::JsonMethodHandler>(
          "POST", null_validator,
          sel::valid_test_config_json_handler, sel::invalid_json_handler);
  auto init_mpc_methodhandler =
      sel::MethodHandler::create_methodhandler<sel::HeaderMethodHandler>(
          "POST", sel::init_mpc);

  auto testconfig_methodhandler =
      sel::MethodHandler::create_methodhandler<sel::HeaderMethodHandler>(
          "GET", sel::test_configs);
  // Create Ressource on <url/init> and instruct to use the built MethodHandler
  sel::ResourceHandler local_initializer{"/initLocal"};
  local_initializer.add_method(init_local_methodhandler);

  sel::ResourceHandler remote_initializer{"/initRemote/{remote_id: .*}"};
  remote_initializer.add_method(init_remote_methodhandler);

  // Create Ressource on <url/linkRecord> and instruct to use the built MethodHandler
  sel::ResourceHandler linkrecord_handler{"/linkRecord/{remote_id: .*}"};
  linkrecord_handler.add_method(linkrecord_methodhandler);
#ifdef SEL_MATCHING_MODE
  sel::ResourceHandler matchrecord_handler{"/matchRecord/{remote_id: .*}"};
  matchrecord_handler.add_method(matchrecord_methodhandler);
#endif
  sel::ResourceHandler testconfig_handler{"/test/{parameter: .*}"};
  testconfig_handler.add_method(testconfig_methodhandler);
  // Create Ressource on <url/jobs> and instruct to use the built MethodHandler
  // The jobid is provided in the url
  sel::ResourceHandler jobmonitor_handler{"/jobs/{job_id: .*}"};
  jobmonitor_handler.add_method(jobmonitor_methodhandler);
  // Ressources for internal usage. Not exposed in public API
  sel::ResourceHandler test_config_handler{"/testConfig/{remote_id: .*}"};
  test_config_handler.add_method(test_config_methodhandler);
  sel::ResourceHandler sellink_handler{"/initMPC/{parameter: .*}"};
  sellink_handler.add_method(init_mpc_methodhandler);

  // Setup REST Server
  auto settings = std::make_shared<restbed::Settings>();
  settings->set_worker_limit(restconf.rest_worker);
  if(restconf.use_ssl){
  // Setup SSL Connection
  auto ssl_settings = std::make_shared<restbed::SSLSettings>();
    ssl_settings->set_http_disabled(true);
    ssl_settings->set_private_key(restbed::Uri(
      "file://" + restconf.ssl_key_file.string()));
    ssl_settings->set_certificate(restbed::Uri(
      "file://" + restconf.ssl_cert_file.string()));
    ssl_settings->set_temporary_diffie_hellman(restbed::Uri(
      "file://" + restconf.ssl_dh_file.string()));
    ssl_settings->set_port(restconf.server_port);
    ssl_settings->set_bind_address(restconf.bind_address);
    settings->set_ssl_settings(ssl_settings);
  } else {
    settings->set_bind_address(restconf.bind_address);
    settings->set_port(restconf.server_port);
  }

  local_initializer.publish(service);
  remote_initializer.publish(service);
  linkrecord_handler.publish(service);
#ifdef SEL_MATCHING_MODE
  matchrecord_handler.publish(service);
#endif
  jobmonitor_handler.publish(service);
  test_config_handler.publish(service);
  testconfig_handler.publish(service);
  sellink_handler.publish(service);
  logger->info("Service Running\n");
  service.start(settings);  // Eventloop

  spdlog::drop_all();
  return 0;
}
