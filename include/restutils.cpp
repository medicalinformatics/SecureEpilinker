#include "restutils.h"
#include "apikeyconfig.hpp"
#include "epilink_input.h"
#include "epilink_result.hpp"
#include "seltypes.h"
#include "util.h"
#include "logger.h"
#include "localconfiguration.h"
#include "remoteconfiguration.h"
#include <curlpp/Easy.hpp>
#include <curlpp/Infos.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>
#include <tuple>
#include <map>
#include <iostream>
#include <sstream>
#include <future>
#include <algorithm>

using namespace std;
namespace sel {

template <> bool check_json_type<bool>(const nlohmann::json& j) {
  return j.is_boolean();
}
template <> bool check_json_type<std::string>(const nlohmann::json& j) {
  return j.is_string();
}

template <> bool check_json_type<size_t>(const nlohmann::json& j) {
  return j.is_number_unsigned();
}

template <> bool check_json_type<uint32_t>(const nlohmann::json& j) {
  return check_json_type<size_t>(j);
}

template <> bool check_json_type<uint16_t>(const nlohmann::json& j) {
  return check_json_type<size_t>(j);
}

template <> std::set<Port> get_checked_result<std::set<Port>>(const nlohmann::json& j, const std::string& field_name){
  std::set<Port> result;
  if(j.at(field_name).is_array()){
    for(const auto& p : j.at(field_name)){
      if(check_json_type<Port>(p)){
        result.emplace(p.get<Port>());
      } else {
        throw std::runtime_error("Field is not a valid port");
      }
    }
  } else {
    throw std::runtime_error("Field is not an array");
  }
  return result;
}

void throw_if_nonexisting_file(const filesystem::path& file) {
  if(!filesystem::exists(file))
    throw runtime_error(file.string() + ": file or folder does not exist!"s);
}

void test_server_config_paths(const ServerConfig& config) {
  throw_if_nonexisting_file(config.local_init_schema_file);
  throw_if_nonexisting_file(config.remote_init_schema_file);
  throw_if_nonexisting_file(config.link_record_schema_file);
  throw_if_nonexisting_file(config.ssl_key_file);
  throw_if_nonexisting_file(config.ssl_cert_file);
  throw_if_nonexisting_file(config.ssl_dh_file);
  throw_if_nonexisting_file(config.log_file);
  throw_if_nonexisting_file(config.circuit_directory);
}

ServerConfig parse_json_server_config(const nlohmann::json& json) {
  BooleanSharing boolean_sharing;
  string sharing_type{get_checked_result<string>(json,"booleanSharing")};
  // Convert sharing type to uppercase to allow both lower and uppercase in
  // config file
  transform(sharing_type.begin(), sharing_type.end(), sharing_type.begin(), ::toupper);
  boolean_sharing = (sharing_type == "YAO") ? BooleanSharing::YAO : BooleanSharing::GMW;
  auto aby_ports{get_checked_result<set<Port>>(json,"abyPorts")};
  ServerConfig result{get_checked_result<string>(json,"localInitSchemaPath"),
          get_checked_result<string>(json,"remoteInitSchemaPath"),
          get_checked_result<string>(json,"linkRecordSchemaPath"),
          get_checked_result<string>(json,"serverKeyPath"),
          get_checked_result<string>(json,"serverCertificatePath"),
          get_checked_result<string>(json,"serverDHPath"),
          get_checked_result<string>(json,"logFilePath"),
          get_checked_result<string>(json,"circuitDirectory"),
          get_checked_result<bool>(json,"useSSL"),
          get_checked_result<Port>(json,"port"),
          get_checked_result<string>(json,"bindAddress"),
          get_checked_result<size_t>(json,"restWorkerThreads"),
          get_checked_result<size_t>(json,"defaultPageSize"),
          get_checked_result<uint32_t>(json,"abyThreads"),
          boolean_sharing,
          aby_ports};
  test_server_config_paths(result);
  return result;
}

unique_ptr<AuthenticationConfig> parse_json_auth_config(const nlohmann::json& j) {
  auto l_auth_type{str_to_authtype(j["authType"].get<string>())};
  if (l_auth_type == AuthenticationType::API_KEY) {
    return AuthenticationConfig::create_authentication<APIKeyConfig>(
        l_auth_type, j["sharedKey"].get<string>());
  }
  return make_unique<AuthenticationConfig>(AuthenticationType::NONE);
}

SessionResponse perform_post_request(string url, string data, list<string> headers, bool get_headers){
  auto logger{get_default_logger()};
  curlpp::Easy curl_request;
  promise<stringstream> response_promise;
  future<stringstream> response_stream{response_promise.get_future()};
  headers.emplace_back("Expect:");
  headers.emplace_back("Content-Length: "s+to_string(data.length()));
  curl_request.setOpt(new curlpp::Options::HttpHeader(headers));
  curl_request.setOpt(new curlpp::Options::Url(url));
  curl_request.setOpt(new curlpp::Options::Verbose(false));
  curl_request.setOpt(new curlpp::Options::Post(true));
  curl_request.setOpt(new curlpp::Options::PostFields(data.c_str()));
  curl_request.setOpt(new curlpp::Options::PostFieldSize(data.size()));
  curl_request.setOpt(new curlpp::Options::SslVerifyHost(false));
  curl_request.setOpt(new curlpp::Options::SslVerifyPeer(false));
  curl_request.setOpt(new curlpp::Options::Header(get_headers));
  send_curl(curl_request, move(response_promise));
  response_stream.wait();
  auto responsecode = curlpp::Infos::ResponseCode::get(curl_request);
  auto stream = response_stream.get();
  return {static_cast<int>(responsecode), stream.str(),{}};
}

SessionResponse perform_get_request(string url, list<string> headers, bool get_headers){
  auto logger{get_default_logger()};
  curlpp::Easy curl_request;
  promise<stringstream> response_promise;
  future<stringstream> response_stream{response_promise.get_future()};
  headers.emplace_back("Expect:");
  curl_request.setOpt(new curlpp::Options::HttpHeader(headers));
  curl_request.setOpt(new curlpp::Options::Url(url));
  curl_request.setOpt(new curlpp::Options::Verbose(false));
  curl_request.setOpt(new curlpp::Options::HttpGet(true));
  curl_request.setOpt(new curlpp::Options::SslVerifyHost(false));
  curl_request.setOpt(new curlpp::Options::SslVerifyPeer(false));
  curl_request.setOpt(new curlpp::Options::Header(get_headers));
  send_curl(curl_request, move(response_promise));
  response_stream.wait();
  auto responsecode = curlpp::Infos::ResponseCode::get(curl_request);
  auto stream = response_stream.get();
  return {static_cast<int>(responsecode), stream.str(),{}};
}

  SessionResponse send_result_to_linkageservice(const vector<Result<CircUnit>>& share,
    optional<vector<string>> ids, const string& role,
    const shared_ptr<const LocalConfiguration>& local_config,
    const shared_ptr<const RemoteConfiguration>& remote_config) {
  auto logger{get_default_logger()};
  nlohmann::json json_data;
  json_data["role"] = role;
  if(share.size() == 1){ // Client sent one record
    json_data["result"] = {{"match", share.front().match},
    {"tentative_match", share.front().tmatch},
    {"bestIndex", share.front().index}};
  } else { // Client sent whole database
  nlohmann::json results;
  for(auto& result : share){
    results.push_back({{"match", result.match},
                       {"tentative_match", result.tmatch},
                       {"bestIndex", result.index}});
  }
  json_data["result"] = results;
}
if(role=="server") {
  if(ids) {
    json_data["ids"] = ids.value();
  } else {
    throw runtime_error("Missing IDs from server result");
  }
}
  auto data{json_data.dump()};
  logger->trace("Data for linkage Service: {}",data);
  list<string> headers{"Content-Type: application/json","Authorization: "s+remote_config->get_linkage_service()->authenticator.sign_transaction("")};
  string url = remote_config->get_linkage_service()->url+"/linkageResult/"+local_config->get_local_id()+'/'+remote_config->get_id();
  logger->debug("Sending {} result to linkage service at {}", role, url);
  auto response{perform_post_request(url, data, headers, false)};
  logger->trace("Linkage service reply: {} - {}", response.return_code, response.body);
  return response;
}
vector<string> get_headers(istream& is,const string& header){
  vector<string> responsevec;
  string line;
  while(safeGetline(is,line)){
    responsevec.emplace_back(line);
  }
  vector<string> headers;
  for(const auto& line : responsevec){
    if(auto pos = line.find(header); pos != string::npos){
      headers.emplace_back(line.substr(header.length()+2));
    }
  }
  return headers;
}
vector<string> get_headers(const string& is,const string& header){
  stringstream stream;
  stream << is;
  return get_headers(stream, header);
  }

string assemble_remote_url(RemoteConfiguration const *remote_config) {
  return remote_config->get_remote_scheme() + "://" + remote_config->get_remote_host() + ':' + to_string(remote_config->get_remote_signaling_port());
}
string assemble_remote_url(const shared_ptr<const RemoteConfiguration>& remote_config) {
  return assemble_remote_url(remote_config.get());
}

void send_curl(curlpp::Easy& request, std::promise<std::stringstream> barrier){
  std::stringstream response;
  request.setOpt(new curlpp::Options::WriteStream(&response));
  request.perform();
  barrier.set_value(move(response));
}

}  // namespace sel
