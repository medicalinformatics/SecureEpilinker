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
#include "jsonutils.h"
#include "restresponses.hpp"
#include "serverhandler.h"
#include "util.h"
#include "valijson/validation_results.hpp"

using namespace std;
namespace sel {
ServerConfig make_server_config_from_json(nlohmann::json json) {
  e_sharing boolean_sharing;
  (json.at("booleanSharing").get<string>() == "yao") ? boolean_sharing = S_YAO
                                                     : boolean_sharing = S_BOOL;
  std::set<Port> aby_ports;
  for (auto p : json.at("abyPorts")) {
    aby_ports.emplace(p.get<Port>());
  }
  return {json.at("localInitSchemaPath").get<std::string>(),
          json.at("remoteInitSchemaPath").get<std::string>(),
          json.at("linkRecordSchemaPath").get<std::string>(),
          json.at("serverKeyPath").get<std::string>(),
          json.at("serverCertificatePath").get<std::string>(),
          json.at("serverDHPath").get<std::string>(),
          json.at("logFilePath").get<std::string>(),
          json.at("useSSL").get<bool>(),
          json.at("port").get<Port>(),
          json.at("bindAddress").get<string>(),
          json.at("restWorkerThreads").get<size_t>(),
          json.at("defaultPageSize").get<size_t>(),
          json.at("abyThreads").get<uint32_t>(),
          boolean_sharing,
          aby_ports};
}


unique_ptr<AuthenticationConfig> get_auth_object(const nlohmann::json& j) {
  auto l_auth_type{str_to_authtype(j["authType"].get<string>())};
  if (l_auth_type == AuthenticationType::API_KEY) {
    return AuthenticationConfig::create_authentication<APIKeyConfig>(
        l_auth_type, j["sharedKey"].get<string>());
  }
  return make_unique<AuthenticationConfig>(AuthenticationType::NONE);
}

SessionResponse valid_test_config_json_handler(
    const nlohmann::json& client_config,
    const RemoteId& remote_id) {
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

SessionResponse valid_linkrecord_json_handler(
    const nlohmann::json& j,
    const RemoteId& remote_id) {
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

        auto data = parse_json_fields(local_config->get_fields(), j.at("fields"));
        job->add_data(move(data));

        server_handler.add_linkage_job(remote_id, move(job));
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

SessionResponse valid_init_remote_json_handler(
    const nlohmann::json& j,
    RemoteId remote_id) {
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
    con.authentication =
        get_auth_object(j.at("connectionProfile").at("authentication"));
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
      linkage_service.authentication =
          get_auth_object(j.at("linkageService").at("authentication"));
      remote_config->set_linkage_service(move(linkage_service));
    }
  } catch (const exception& e) {
    logger->error("Error creating remote config: {}", e.what());
    return responses::status_error(restbed::INTERNAL_SERVER_ERROR, e.what());
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
    RemoteId) {
  auto logger = get_default_logger();
  auto& config_handler{ConfigurationHandler::get()};
  logger->trace("Payload: {}", j.dump(2));
  auto local_config = make_shared<LocalConfiguration>();
  logger->info("Creating local configuration\n");
  try {
    local_config->set_local_id(j.at("localId"));
    auto l_auth = get_auth_object(j.at("localAuthentication"));
    local_config->set_local_auth(move(l_auth));
    auto url = j.at("dataService").at("url").get<string>();
    local_config->set_data_service(move(url));

    // Epilink Config
    auto algo_json = j.at("algorithm");
    EpilinkConfig epilink_config = parse_json_epilink_config(algo_json);
    local_config->set_epilink_config(epilink_config);
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
