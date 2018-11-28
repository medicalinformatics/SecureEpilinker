/**
\file    restutils.h
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
  \brief utility functions regarding the secure epilinker rest interface
*/

#ifndef SEL_RESTUTILS_H
#define SEL_RESTUTILS_H

#include "resttypes.h"
#include "circuit_config.h" // CircUnit
#include <memory>
#include <optional>

#include <future>
#include <sstream>

#include <nlohmann/json.hpp>
#include <curlpp/Easy.hpp>

namespace sel {

class RemoteConfiguration;
class LocalConfiguration;
template<typename T> struct Result;

template <typename T> bool check_json_type(const nlohmann::json& j);
template <> bool check_json_type<bool>(const nlohmann::json& j);
template <> bool check_json_type<std::string>(const nlohmann::json& j);
template <> bool check_json_type<size_t>(const nlohmann::json& j);
template <> bool check_json_type<uint32_t>(const nlohmann::json& j);
template <> bool check_json_type<uint16_t>(const nlohmann::json& j);

template <typename T>
T get_checked_result(const nlohmann::json& j, const std::string& field_name){
  if(check_json_type<T>(j.at(field_name))){
    return j.at(field_name).get<T>();
  } else
    throw std::runtime_error("Wrong type in config");
}

template <> std::set<Port> get_checked_result<std::set<Port>>(const nlohmann::json& j, const std::string& field_name);

void throw_if_nonexisting_file(const std::filesystem::path&);
void test_server_config_paths(const ServerConfig&);

ServerConfig parse_json_server_config(const nlohmann::json&);
std::unique_ptr<AuthenticationConfig> parse_json_auth_config(const nlohmann::json&);

std::string assemble_remote_url(const std::shared_ptr<const RemoteConfiguration>&);
std::string assemble_remote_url(RemoteConfiguration const * );
SessionResponse perform_post_request(std::string, std::string, std::list<std::string>, bool);
SessionResponse perform_get_request(std::string, std::list<std::string>, bool);
SessionResponse send_result_to_linkageservice(const std::vector<Result<CircUnit>>&, std::optional<std::vector<std::string> >,const std::string&,const std::shared_ptr<const LocalConfiguration>&,const std::shared_ptr<const RemoteConfiguration>&);
std::vector<std::string> get_headers(std::istream& is,const std::string& header);
std::vector<std::string> get_headers(const std::string&,const std::string& header);

std::stringstream send_curl(curlpp::Easy& request);

} // namespace sel
#endif /* end of include guard: SEL_RESTUTILS_H */
