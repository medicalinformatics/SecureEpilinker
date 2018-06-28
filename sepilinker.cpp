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
#include "include/validator.h"
#include "include/jsonhandlerfunctions.h"
#include "include/tempselmethodhandler.h"

#include "fmt/format.h"
#include "nlohmann/json.hpp"
#include "cxxopts.hpp"
#include "restbed"

#include <functional>
#include <memory>
#include <string>
#include <curlpp/cURLpp.hpp>

using json = nlohmann::json;
using namespace sel;


int main(int argc, char* argv[]) {
  // Commandline Parser
  cxxopts::Options options("Secure EpiLinker", "Secure Multi Party Recard Linkage via EpiLink algorithm, Version 0.1.0");
  options.add_options()
    ("c,config", "Config file name", cxxopts::value<std::string>()->default_value("../data/serverconf.json"))
    ("i,initschema", "File name of initializeation schema", cxxopts::value<std::string>())
    ("l,linkschema", "File name of linkRecord schema", cxxopts::value<std::string>())
    ("k,key", "File name of server key", cxxopts::value<std::string>())
    ("d,dh", "File name of Diffi-Hellman group", cxxopts::value<std::string>())
    ("C,cert", "File name of server certificate", cxxopts::value<std::string>())
    ("p,port", "Port for listening", cxxopts::value<unsigned>())
    ("h,help", " Print this help");
  auto cmdoptions{options.parse(argc,argv)};
  if(cmdoptions.count("help")) {
    fmt::print("{}\n",options.help());
    return 0;
  }
  auto server_config = read_json_from_disk(cmdoptions["config"].as<std::string>());

  // Override config file
  if(cmdoptions.count("initschema")){
    server_config["initSchemaPath"] = cmdoptions["initschema"].as<std::string>();
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
  
  // Program
  restbed::Service service;
  curlpp::Cleanup curl_cleanup;
  // Create Connection Handler
  auto connections = std::make_shared<sel::ConnectionHandler>(&service);
  auto configurations = std::make_shared<sel::ConfigurationHandler>(connections);
  auto data = std::make_shared<sel::DataHandler>();
  auto servers = std::make_shared<sel::ServerHandler>(configurations, data);

  connections->set_config_handler(configurations);
  data->set_config_handler(configurations);
  // Create JSON Validator
  auto init_validator = std::make_shared<sel::Validator>(
      read_json_from_disk(server_config["initSchemaPath"].get<std::string>()));
  auto linkrecord_validator =
      std::make_shared<sel::Validator>(read_json_from_disk(
          server_config["linkRecordSchemaPath"].get<std::string>()));
  // Create Handler for POST Request with JSON payload (using the validator)
  auto init_methodhandler =
      sel::MethodHandler::create_methodhandler<sel::JsonMethodHandler>(
          "PUT", configurations, connections, servers,init_validator);
  auto linkrecord_methodhandler =
      sel::MethodHandler::create_methodhandler<sel::JsonMethodHandler>(
          "POST", configurations, connections, servers, linkrecord_validator);
  // Create GET-Handler for job status monitoring
  auto jobmonitor_methodhandler =
      sel::MethodHandler::create_methodhandler<sel::MonitorMethodHandler>(
          "GET", servers);
  auto temp_selconnect_methodhandler =
      sel::MethodHandler::create_methodhandler<sel::TempSelMethodHandler>(
          "POST", connections, servers, data);

  // Add Validation Callbacks
  auto devptr_init =
      dynamic_cast<sel::JsonMethodHandler*>(init_methodhandler.get());
  devptr_init->set_valid_callback(sel::valid_init_json_handler);
  devptr_init->set_invalid_callback(sel::invalid_json_handler);
  auto devptr_linkrecord =
      dynamic_cast<sel::JsonMethodHandler*>(linkrecord_methodhandler.get());
  devptr_linkrecord->set_valid_callback(valid_linkrecord_json_handler);
  devptr_linkrecord->set_invalid_callback(invalid_json_handler);
  // Create Ressource on <url/init> and instruct to use the built MethodHandler
  sel::ResourceHandler initializer{"/init/{remote_id: .*}"};
  initializer.add_method(init_methodhandler);

  // Create Ressource on <url/linkRecord> and instruct to use the built MethodHandler
  sel::ResourceHandler linkrecord_handler{"/linkRecord/{remote_id: .*}"};
  linkrecord_handler.add_method(linkrecord_methodhandler);
  // Create Ressource on <url/jobs> and instruct to use the built MethodHandler
  // The jobid is provided in the url
  sel::ResourceHandler jobmonitor_handler{"/jobs/{job_id: .*}"};
  jobmonitor_handler.add_method(jobmonitor_methodhandler);
  sel::ResourceHandler selconnect_handler{"/selconnect"};
  selconnect_handler.add_method(temp_selconnect_methodhandler);
  // Setup SSL Connection
  auto ssl_settings = std::make_shared<restbed::SSLSettings>();
  ssl_settings->set_http_disabled(true);
  ssl_settings->set_private_key(restbed::Uri(
      "file://" + server_config["serverKeyPath"].get<std::string>()));
  ssl_settings->set_certificate(restbed::Uri(
      "file://" + server_config["serverCertificatePath"].get<std::string>()));
  ssl_settings->set_temporary_diffie_hellman(restbed::Uri(
      "file://" + server_config["serverDHPath"].get<std::string>()));

  // Setup REST Server
  auto settings = std::make_shared<restbed::Settings>();
  ssl_settings->set_port(server_config["port"].get<unsigned>());
  ssl_settings->set_bind_address("0.0.0.0");
  settings->set_worker_limit( 4 );
  if(server_config["useSSL"].get<bool>()){
    settings->set_ssl_settings(ssl_settings);
  } else {
    settings->set_port(8080);
  }

  initializer.publish(service);
  linkrecord_handler.publish(service);
  jobmonitor_handler.publish(service);
  selconnect_handler.publish(service);
  fmt::print("Service Running\n");
  service.start(settings);  // Eventloop

  return 0;
}
