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

#include <experimental/filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include "apikeyconfig.hpp"
#include "authenticationconfig.hpp"
#include "base64.h"
#include "connectionhandler.h"
#include "serverhandler.h"
#include "fmt/format.h"
#include "localconfiguration.h"
#include "nlohmann/json.hpp"
#include "remoteconfiguration.h"
#include "restbed"
#include "seltypes.h"
#include "util.h"
#include "valijson/validation_results.hpp"

using namespace std;
namespace sel {
nlohmann::json read_json_from_disk(
    const experimental::filesystem::path& json_path) {
  nlohmann::json content;
  if (experimental::filesystem::exists(json_path)) {
    ifstream in(json_path);
    try {
      in >> content;
    } catch (const exception& e) {
      fmt::print(stderr, "Error {}! Terminating!\n", e.what());
      exit(1);
    }
    return content;
  } else {
    throw runtime_error(json_path.string() + " does not exist!");
    return content;  // To silence warning, will never be executed
  }
}

unique_ptr<AuthenticationConfig> get_auth_object(const nlohmann::json& j) {
  auto l_auth_type{str_to_authtype(j["authType"].get<string>())};
  if (l_auth_type == AuthenticationType::API_KEY) {
    return AuthenticationConfig::create_authentication<APIKeyConfig>(
        l_auth_type, j["sharedkey"].get<string>());
  }
  return make_unique<AuthenticationConfig>(AuthenticationType::NONE);
}

bool check_exchange_group(const IndexSet& fieldnames,
                          const IndexSet& exchange_group) {
  bool fields_present{true};
  for (const auto& f : exchange_group) {
    fields_present &= fieldnames.count(f);
  }
  return fields_present;
}

SessionResponse valid_linkrecord_json_handler(
    const nlohmann::json& j,
    const RemoteId& remote_id,
    const shared_ptr<ConfigurationHandler>& config_handler,
    const shared_ptr<ServerHandler>& server_handler,
    const shared_ptr<ConnectionHandler>&) {
  try {
    JobId job_id;
    if (config_handler->get_remote_count()) {
      const auto local_config{config_handler->get_local_config()};
      const auto remote_config{config_handler->get_remote_config(remote_id)};
      const auto algo_config{config_handler->get_algorithm_config()};
      auto job{make_shared<LinkageJob>(
          local_config,
          remote_config,
          algo_config, server_handler)};
      job_id = job->get_id();
      fmt::print("Ressource Path: {}\n", job_id);
      try {
        CallbackConfig callback;
        callback.url = j["callback"]["url"].get<string>();
        callback.token = j["callback"]["token"].get<string>();
        job->set_callback(move(callback));

        for (auto f = j["fields"].begin(); f != j["fields"].end(); ++f) {
          const auto& field_config = local_config->get_field(f.key());
          DataField tempfield;
          switch (field_config.type) {
            case FieldType::BITMASK: {
              if (!(f->is_string())) {
                throw runtime_error("Invalid Field Type");
              }
              const auto b64string{f->get<string>()};
              auto tempbytearray{base64_decode(b64string)};
              if (!check_bloom_length(tempbytearray,
                                      algo_config->bloom_length)) {
                fmt::print(
                    "Warning: Set bits after bloomfilterlength. Set to "
                    "zero.\n");
              }
              tempfield = tempbytearray;
              break;
            }
            case FieldType::INTEGER: {
              if (!(f->is_number_integer())) {
                throw runtime_error("Invalid Field Type");
              }
              tempfield = f->get<int>();
              break;
            }
            case FieldType::NUMBER: {
              if (!(f->is_number())) {
                throw runtime_error("Invalid Field Type");
              }
              tempfield = f->get<double>();
              break;
            }
            case FieldType::STRING: {
              if (!(f->is_string())) {
                throw runtime_error("Invalid Field Type");
              }
              tempfield = f->get<string>();
              break;
            }
            default: { throw runtime_error("Invalid Field Type"); }
          }  // Switch
            job->add_data_field(field_config.name, move(tempfield));
        }

        server_handler->add_linkage_job(remote_id, move(job));
      } catch (const exception& e) {
        fmt::print(stderr, "Error: {}\n", e.what());
        return {restbed::BAD_REQUEST,
                e.what(),
                {{"Content-Length", to_string(string(e.what()).length())},
                 {"Connection", "Close"}}};
      }
    } else {
      return {restbed::UNAUTHORIZED,
              "No connection initialized",
              {{"Content-Length", "25"}, {"Connection", "Close"}}};
    }
    return {restbed::ACCEPTED,
            "Job Queued",
            {{"Content-Length", "10"},
             {"Connection", "Close"},
             {"Location", "/jobs/" + job_id}}};
  } catch (const runtime_error& e) {
    return {restbed::INTERNAL_SERVER_ERROR,
            e.what(),
            {{"Content-Length", to_string(string(e.what()).length())},
             {"Connection", "Close"}}};
  }
}

SessionResponse valid_init_json_handler(
    const nlohmann::json& j,
    RemoteId remote_id,
    const shared_ptr<ConfigurationHandler>& config_handler,
    const shared_ptr<ServerHandler>& server_handler,
    const shared_ptr<ConnectionHandler>& connection_handler) {
  if (remote_id == "local") {
    auto local_config = make_shared<LocalConfiguration>();
    fmt::print("Creating local configuration\n");
    vector<ML_Field> fields;
    auto algo{make_shared<AlgorithmConfig>()};
    try {
      // Get local Authentication
      auto l_auth{get_auth_object(j["localAuthentication"])};
      local_config->set_local_auth(move(l_auth));
      // Get Dataservice
      auto url{j["dataService"]["url"].get<string>()};
      local_config->set_data_service(move(url));
      // Get Field Descriptions
      IndexSet fieldnames;
      for (const auto& f : j["algorithm"]["fields"]) {
        ML_Field tempfield(
            f["name"].get<string>(), f["frequency"].get<double>(),
            f["errorRate"].get<double>(), f["comparator"].get<string>(),
            f["fieldType"].get<string>(),5); //TODO(TK) implement API change
        local_config->add_field(move(tempfield));
        fieldnames.emplace(f["name"].get<string>());
      }
      for (const auto& eg : j["algorithm"]["exchangeGroups"]) {
        IndexSet egroup;
        for (const auto& f : eg) {
          egroup.emplace(f.get<string>());
        }
        if (check_exchange_group(fieldnames, egroup)) {
          local_config->add_exchange_group(egroup);
        } else {
          throw runtime_error("Invalid Exchange Group! Field doesn't exist!");
        }
      }
      config_handler->set_local_config(move(local_config));
      // Get Algorithm Config
      algo->type = str_to_atype(j["algorithm"]["algoType"].get<string>());
      algo->bloom_length = j["algorithm"]["bloomLength"].get<unsigned>();
      algo->threshold_match = j["algorithm"]["threshold_match"].get<double>();
      algo->threshold_non_match =
          j["algorithm"]["threshold_non_match"].get<double>();
      config_handler->set_algorithm_config(move(algo));
      return {restbed::OK, "", {{"Connection", "Close"}}};
    } catch (const runtime_error& e) {
      return {restbed::INTERNAL_SERVER_ERROR,
              e.what(),
              {{"Content-Length", to_string(string(e.what()).length())}}};
    }
  } else {  // remote Config
    fmt::print("Creating remote Config for: \"{}\"\n", remote_id);
      ConnectionConfig con;
      ConnectionConfig linkage_service;
      auto local_conf{config_handler->get_local_config()};
      auto algo_conf{config_handler->get_algorithm_config()};
    try {
      // Get Connection Profile
      con.url = j["connectionProfile"]["url"].get<string>();
      con.authentication =
          get_auth_object(j["connectionProfile"]["authentication"]);
      // Get Linkage Service Config
      linkage_service.url = j["linkageService"]["url"].get<string>();
      linkage_service.authentication =
          get_auth_object(j["linkageService"]["authentication"]);
    } catch (const runtime_error& e) {
      return {restbed::INTERNAL_SERVER_ERROR,
              e.what(),
              {{"Content-Length", to_string(string(e.what()).length())}}};
    }
    auto remote_config = make_shared<RemoteConfiguration>(remote_id);
    remote_config->set_connection_profile(move(con));
    remote_config->set_linkage_service(move(linkage_service));
    auto remote_info{connection_handler->initialize_aby_server(remote_config)};
    remote_config->set_aby_port(remote_info.port);
    connection_handler->mark_port_used(remote_info.port);
    remote_config->set_remote_client_id(remote_info.id);
    config_handler->set_remote_config(move(remote_config));
    auto fun = bind(&ServerHandler::insert_client, server_handler.get(), placeholders::_1);
    std::thread client_creator(fun, remote_id);
    client_creator.detach();

    return {restbed::OK, "", {{"Connection", "Close"}}};
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
  return {
      restbed::BAD_REQUEST,
      err,
      {{"Content-Length", to_string(err.length())}, {"Connection", "Close"}}};
}
}  // namespace sel
