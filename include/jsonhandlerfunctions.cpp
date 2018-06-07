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
#include <unordered_set>
#include <vector>
#include "apikeyconfig.hpp"
#include "authenticationconfig.hpp"
#include "base64.h"
#include "connectionhandler.h"
#include "fmt/format.h"
#include "localconfiguration.h"
#include "nlohmann/json.hpp"
#include "remoteconfiguration.h"
#include "restbed"
#include "seltypes.h"
#include "valijson/validation_results.hpp"
#include "util.h"

using namespace sel;
nlohmann::json sel::read_json_from_disk(
    const std::experimental::filesystem::path& json_path) {
  nlohmann::json content;
  if (std::experimental::filesystem::exists(json_path)) {
    std::ifstream in(json_path);
    try {
      in >> content;
    } catch (const std::exception& e) {
      fmt::print(stderr, "Error {}! Terminating!\n", e.what());
      exit(1);
    }
    return content;
  } else {
    throw std::runtime_error(json_path.string() + " does not exist!");
    return content;  // To silence warning, will never be executed
  }
}

std::unique_ptr<sel::AuthenticationConfig> sel::get_auth_object(
    const nlohmann::json& j) {
  auto l_auth_type{sel::str_to_authtype(j["authType"].get<std::string>())};
  if (l_auth_type == sel::AuthenticationType::API_KEY) {
    return sel::AuthenticationConfig::create_authentication<sel::APIKeyConfig>(
        l_auth_type, j["sharedkey"].get<std::string>());
  }
  return std::make_unique<sel::AuthenticationConfig>(
      sel::AuthenticationType::NONE);
}

bool sel::check_exchange_group(
    const std::unordered_set<sel::FieldName>& fieldnames,
    const std::set<sel::FieldName>& exchange_group) {
  bool fields_present{true};
  for (const auto& f : exchange_group) {
    fields_present &= fieldnames.count(f);
  }
  return fields_present;
}

sel::SessionResponse sel::valid_linkrecord_json_handler(
    const nlohmann::json& j,
    const sel::RemoteId& remote_id,
    const std::shared_ptr<sel::ConnectionHandler>& handler) {
  try {
    sel::JobId job_id;
    if (handler->num_connections()) {
      auto job{std::make_shared<LinkageJob>(handler->get_local_configuration(), handler->get_remote_configuration(remote_id))};
      job_id = job->get_id();
      fmt::print("Ressource Path: {}\n", job_id);
      try {
        CallbackConfig callback;
        callback.url = j["callback"]["url"].get<std::string>();
        callback.token = j["callback"]["token"].get<std::string>();
        job->set_callback(std::move(callback));

        for (auto f = j["fields"].begin(); f != j["fields"].end(); ++f) {
          const auto& field_config = handler->get_field(f.key());
          DataField tempfield;
          bool empty{false};
          switch (field_config.type) {
            case FieldType::BITMASK: {
              if (!(f->is_string())) {
                throw std::runtime_error("Invalid Field Type");
              }
              const auto b64string{f->get<std::string>()};
              if(sel::trim_copy(b64string) == "") empty = true;
              auto tempbytearray{base64_decode(b64string)};
//              fmt::print("Bitstring: {}\n", print_bytearray(tempbytearray));
              if(!check_bloom_length(tempbytearray,(handler->get_local_configuration())->get_bloom_length())){
            fmt::print(
                "Warning: Set bits after bloomfilterlength. Set to zero.\n");
              } 
              tempfield = tempbytearray;
              break;
            }
            case FieldType::INTEGER: {
              if (!(f->is_number_integer())) {
                throw std::runtime_error("Invalid Field Type");
              }
              tempfield = f->get<int>();
              if(!f->get<int>()) empty = true;
              break;
            }
            case FieldType::NUMBER: {
              if (!(f->is_number())) {
                throw std::runtime_error("Invalid Field Type");
              }
              tempfield = f->get<double>();
              if(!f->get<double>()) empty = true;
              break;
            }
            case FieldType::STRING: {
              if (!(f->is_string())) {
                throw std::runtime_error("Invalid Field Type");
              }
              tempfield = f->get<std::string>();
              if(sel::trim_copy(f->get<std::string>()) == "") empty = true;
              break;
            }
            default: { throw std::runtime_error("Invalid Field Type"); }
          }  // Switch
          if(field_config.comparator == sel::FieldComparator::NGRAM){
            job->add_hw_data_field(field_config.name, std::move(tempfield), empty);
          } else {
            job->add_bin_data_field(field_config.name, std::move(tempfield), empty);
          }
        }

        handler->add_job(remote_id, std::move(job));
      } catch (const std::exception& e) {
        fmt::print(stderr, "Error: {}\n", e.what());
        return {
            restbed::BAD_REQUEST,
            e.what(),
            {{"Content-Length", std::to_string(std::string(e.what()).length())},
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
  } catch (const std::runtime_error& e) {
    return {restbed::INTERNAL_SERVER_ERROR,
            e.what(),
            {{"Content-Length", std::to_string(std::string(e.what()).length())},
             {"Connection", "Close"}}};
  }
}

sel::SessionResponse sel::valid_init_json_handler(
    const nlohmann::json& j,
    sel::RemoteId remote_id,
    const std::shared_ptr<sel::ConnectionHandler>& handler) {
  if (remote_id == "local") {
    auto local_config = std::make_shared<sel::LocalConfiguration>();
    fmt::print("Creating local configuration\n");
    std::vector<sel::ML_Field> fields;
    sel::AlgorithmConfig algo;
    try {
      // Get local Authentication
      auto l_auth{get_auth_object(j["localAuthentication"])};
      local_config->set_local_auth(std::move(l_auth));
      // Get Dataservice
      auto url{j["dataService"]["url"].get<std::string>()};
      local_config->set_data_service_url(std::move(url));
      // Get Field Descriptions
      std::unordered_set<sel::FieldName> fieldnames;
      for (const auto& f : j["algorithm"]["fields"]) {
        sel::ML_Field tempfield(
            f["name"].get<std::string>(), f["frequency"].get<double>(),
            f["errorRate"].get<double>(), f["comparator"].get<std::string>(),
            f["fieldType"].get<std::string>());
        local_config->add_field(std::move(tempfield));
        fieldnames.emplace(f["name"].get<std::string>());
      }
      for (const auto& eg : j["algorithm"]["exchangeGroups"]) {
        std::set<sel::FieldName> egroup;
        for (const auto& f : eg) {
          egroup.emplace(f.get<std::string>());
        }
        if (check_exchange_group(fieldnames, egroup)) {
          local_config->add_exchange_group(egroup);
        } else {
          throw std::runtime_error(
              "Invalid Exchange Group! Field doesn't exist!");
        }
      }
      // Get Algorithm Config
      algo.type =
          sel::str_to_atype(j["algorithm"]["algoType"].get<std::string>());
      algo.bloom_length = j["algorithm"]["bloomLength"].get<unsigned>();
      algo.threshold_match = j["algorithm"]["threshold_match"].get<double>();
      algo.threshold_non_match =
          j["algorithm"]["threshold_non_match"].get<double>();
      local_config->set_algorithm_config(algo);
      handler->set_local_configuration(std::move(local_config));
      return {restbed::OK, "", {{"Connection", "Close"}}};
    } catch (const std::runtime_error& e) {
      return {
          restbed::INTERNAL_SERVER_ERROR,
          e.what(),
          {{"Content-Length", std::to_string(std::string(e.what()).length())}}};
    }
  } else {  // remote Config
    auto remote_config = std::make_shared<sel::RemoteConfiguration>(remote_id);
    fmt::print("Creating remote Config for: \"{}\"\n", remote_id);
    try {
      sel::ConnectionConfig con;
      sel::ConnectionConfig linkage_service;
      // Get Connection Profile
      con.url = j["connectionProfile"]["url"].get<std::string>();
      con.authentication =
          get_auth_object(j["connectionProfile"]["authentication"]);
      remote_config->set_connection_profile(std::move(con));
      // Get Linkage Service Config
      linkage_service.url = j["linkageService"]["url"].get<std::string>();
      linkage_service.authentication =
          get_auth_object(j["linkageService"]["authentication"]);
      remote_config->set_linkage_service(std::move(linkage_service));
    } catch (const std::runtime_error& e) {
      return {
          restbed::INTERNAL_SERVER_ERROR,
          e.what(),
          {{"Content-Length", std::to_string(std::string(e.what()).length())}}};
    }
    handler->upsert_connection(remote_config);

    return {restbed::OK, "", {{"Connection", "Close"}}};
  }
}

sel::SessionResponse sel::invalid_json_handler(
    valijson::ValidationResults& results) {
  std::string err = "JSON validation failed.\n";
  valijson::ValidationResults::Error error;
  int err_num = 1;
  while (results.popError(error)) {
    std::string context;
    auto it = error.context.begin();
    for (; it != error.context.end(); it++) {
      context += *it;
    }

    err += "Error #" + std::to_string(err_num) + "\n";
    err += " context: " + context + "\n";
    err += " description: " + error.description + "\n";

    ++err_num;
  }
  return {restbed::BAD_REQUEST,
          err,
          {{"Content-Length", std::to_string(err.length())},
           {"Connection", "Close"}}};
}
