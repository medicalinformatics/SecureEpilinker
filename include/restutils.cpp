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

using namespace std;
namespace sel {

ServerConfig parse_json_server_config(const nlohmann::json& json) {
  BooleanSharing boolean_sharing;
  (json.at("booleanSharing").get<string>() == "yao") ? boolean_sharing = BooleanSharing::YAO
                                                     : boolean_sharing = BooleanSharing::GMW;
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

SessionResponse send_result_to_linkageservice(const Result<CircUnit>& share,
    optional<vector<string>> ids, const string& role,
    const shared_ptr<const LocalConfiguration>& local_config,
    const shared_ptr<const RemoteConfiguration>& remote_config) {
  auto logger{get_default_logger()};
  nlohmann::json json_data;
  json_data["role"] = role;
  json_data["result"] = {{"match", share.match},
                          {"tentative_match", share.tmatch},
                          {"bestIndex", share.index}};
  if (role=="server") {
    if(ids) {
      json_data["ids"] = ids.value();
    } else {
      throw runtime_error("Missing IDS from server result");
    }
  }
  auto data{json_data.dump()};
  logger->trace("Data for linkage Service: {}",data);
  list<string> headers{"Content-Type: application/json","Authorization: "s+dynamic_cast<APIKeyConfig*>(remote_config->get_linkage_service()->authentication.get())->get_key()};
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
