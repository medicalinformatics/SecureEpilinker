/**
\file    jsonhandlerfunctions.cpp
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
\brief Functions for handling given JSON Objects
*/

#include "jsonhandlerfunctions.h"

#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include "apikeyconfig.hpp"
#include "authenticationconfig.hpp"
#include "base64.h"
#include "connectionhandler.h"
#include "fmt/format.h"
#include "localconfiguration.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "remoteconfiguration.h"
#include "configurationhandler.h"
#include "restbed"
#include "resttypes.h"
#include "restutils.h"
#include "connectionconfig.hpp"
#include "jsonutils.h"
#include "restresponses.hpp"
#include "serverhandler.h"
#include "util.h"
#include "valijson/validation_results.hpp"

using namespace std;
namespace sel {

SessionResponse valid_test_config_json_handler(
    const nlohmann::json& client_config,
    const RemoteId& remote_id,
    const string& authorization) {
  auto logger{get_default_logger()};
  auto& config_handler{ConfigurationHandler::get()};
  auto& connection_handler{ConnectionHandler::get()};

  SessionResponse response;
  if (config_handler.remote_exists(remote_id)) {
    // negotiate common ABY port
    auto client_ports{client_config.at("availableAbyPorts").get<set<Port>>()};
    Port common_port = connection_handler.choose_common_port(client_ports);
    logger->debug("Common port: {}", common_port);
    // The aby port information is not needed for config comparison
    auto client_comparison_config = client_config;
    client_comparison_config.erase("availableAbyPorts");
    // Compare Configs
    if (config_handler.compare_configuration(client_comparison_config, remote_id)) {
      logger->info("Valid config");
      const auto remote_config{config_handler.get_remote_config(remote_id)};
      remote_config->set_aby_port(common_port);
      remote_config->mark_mutually_initialized();

      logger->info("Building MPC Server");
      RemoteAddress tempadr{remote_config->get_remote_host(),common_port};
      std::thread server_creator([remote_id,tempadr](){ServerHandler::get().insert_server(remote_id, tempadr);});
      server_creator.detach();
      return responses::server_initialized(common_port);
    } else {
      logger->error("Invalid Configs");
      return responses::status_error(restbed::BAD_REQUEST,"Configurations are not compatible");
    }
  } else {
    logger->info("Remote does not exist. Wait for pairing");
    return responses::not_initialized;
  }
}

SessionResponse linkrecords(
    const nlohmann::json& j,
    const RemoteId& remote_id,
    const string& authorization,
    bool multiple_records) {
  auto logger{get_default_logger()};
  const auto& config_handler{ConfigurationHandler::cget()};
  auto& server_handler{ServerHandler::get()};
  try {
    logger->trace("Link/MatchRecord Payload: {}", j.dump(2));
    JobId job_id;
    if (config_handler.get_remote_count()) {
      const auto local_config{config_handler.get_local_config()};
      const auto remote_config{config_handler.get_remote_config(remote_id)};
      auto job{make_shared<LinkageJob>(local_config, remote_config)};
      job_id = job->get_id();
      logger->info("Created Job on Path: {}", job_id);
      try {
        job->set_callback(j.at("callback")
            .at("url")
            .get<string>());  // no move to use copy elision

        VRecord data;
        if(!multiple_records) {
          data = record_to_vrecord(parse_json_fields(local_config->get_fields(), j.at("fields")));
        } else {
          data = DataHandler::cget().get_client_records(j);
        }
        job->add_data(move(data));
        server_handler.add_linkage_job(remote_id, job);
      } catch (const exception& e) {
        logger->error("Error in job creation: {}", e.what());
        return responses::status_error(restbed::BAD_REQUEST,e.what());
      }
    } else {
      return responses::not_initialized;
    }
    return {restbed::ACCEPTED,
      "Job Queued",
      {{"Content-Length", "10"},
        {"Connection", "Close"},
        {"Location", "/jobs/" + job_id}}};
  } catch (const runtime_error& e) {
    return responses::status_error(restbed::INTERNAL_SERVER_ERROR, e.what());
  }
}

SessionResponse valid_linkrecord_json_handler(
    const nlohmann::json& j,
    const RemoteId& remote_id,
    const string& authorization) {
  return linkrecords(j, remote_id, authorization, false);
}

SessionResponse valid_linkrecords_json_handler(
    const nlohmann::json& j,
    const RemoteId& remote_id,
    const string& authorization) {
  return linkrecords(j,remote_id,authorization, true);
}

SessionResponse valid_init_remote_json_handler(
    const nlohmann::json& j,
    const RemoteId& remote_id,
    const string&) {
  auto logger{get_default_logger()};
  auto& config_handler{ConfigurationHandler::get()};
  logger->trace("Payload: {}", j.dump(2));
  logger->info("Creating remote Config for: \"{}\"", remote_id);
  ConnectionConfig con;
  ConnectionConfig linkage_service;
  auto local_conf{config_handler.get_local_config()};
  auto remote_config = make_shared<RemoteConfiguration>(remote_id);
  try {
    // Get Connection Profile
    con.url = j.at("connectionProfile").at("url").get<string>();
    con.authenticator.set_auth_info(move(parse_json_auth_config(j.at("connectionProfile").at("authentication"))));
    remote_config->set_connection_profile(move(con));
    bool matching_mode;
    if (!j.count("matchingAllowed")) {
      matching_mode = false;
    } else {
      matching_mode = j["matchingAllowed"].get<bool>();
      remote_config->set_matching_mode(matching_mode);
#ifndef SEL_MATCHING_MODE
      if (matching_mode) {
        logger->critical(
            "Matching Mode needs compile flag"
            " \"SEL_MATCHING_MODE\" to work. Terminating!");
        exit(3);
      }
#endif
    }

    // Get Linkage Service Config, if not in matching mode
    if (!matching_mode) {
      linkage_service.url = j.at("linkageService").at("url").get<string>();
      linkage_service.authenticator.set_auth_info(parse_json_auth_config(j.at("linkageService").at("authentication")));
      remote_config->set_linkage_service(move(linkage_service));
    }
  } catch (const exception& e) {
    logger->error("Error creating remote config: {}", e.what());
    return responses::status_error(restbed::INTERNAL_SERVER_ERROR, e.what());
  }
  if(config_handler.remote_exists(remote_id)) {
    return responses::status_error(restbed::NOT_IMPLEMENTED, "Updating of configurations is not implemented yet");
  }
  config_handler.set_remote_config(move(remote_config));
  // Test connection and negotiate common port
  //auto placed_config{config_handler.get_remote_config(remote_id)};
  //placed_config->test_configuration(config_handler->get_local_config()->get_local_id(), config_handler->make_comparison_config(remote_id), connection_handler, server_handler);
  // TODO(TK): Error handling
  return {restbed::OK, "", {{"Connection", "Close"}}};
}

SessionResponse valid_init_local_json_handler(
    const nlohmann::json& j,
    const RemoteId&,
    const string&) {
  auto logger = get_default_logger();
  auto& config_handler{ConfigurationHandler::get()};
  logger->trace("Payload: {}", j.dump(2));
  auto local_config = make_shared<LocalConfiguration>();
  logger->info("Creating local configuration\n");
  try {
    local_config->set_local_id(j.at("localId"));
    auto l_auth = parse_json_auth_config(j.at("localAuthentication"));
    local_config->set_local_auth(move(l_auth));
    auto url = j.at("dataService").at("url").get<string>();
    local_config->set_data_service(move(url));

    // Epilink Config
    auto algo_json = j.at("algorithm");
    EpilinkConfig epilink_config = parse_json_epilink_config(algo_json);
    local_config->set_epilink_config(epilink_config);
  
    if(config_handler.get_local_config()) {
      return responses::status_error(restbed::NOT_IMPLEMENTED, "Updating of configurations is not implemented yet");
    }

    config_handler.set_local_config(move(local_config));

    return {restbed::OK, "", {{"Connection", "Close"}}};
  } catch (const runtime_error& e) {
    return responses::status_error(restbed::INTERNAL_SERVER_ERROR, e.what());
  }
}

SessionResponse invalid_json_handler(valijson::ValidationResults& results) {
  string err = "JSON validation failed.\n";
  valijson::ValidationResults::Error error;
  int err_num = 1;
  while (results.popError(error)) {
    string context;
    auto it = error.context.begin();
    for (; it != error.context.end(); it++) {
      context += *it;
    }

    err += "Error #" + to_string(err_num) + "\n";
    err += " context: " + context + "\n";
    err += " description: " + error.description + "\n";

    ++err_num;
  }
  return responses::status_error( restbed::BAD_REQUEST, err);
}
}  // namespace sel
