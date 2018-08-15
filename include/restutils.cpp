#include "restutils.h"
#include <tuple>
#include <map>
#include "apikeyconfig.hpp"
#include "epilink_input.h"
#include "nlohmann/json.hpp"
#include "seltypes.h"
#include "util.h"
#include "base64.h"
#include "logger.h"
#include "localconfiguration.h"
#include "remoteconfiguration.h"
#include <experimental/filesystem>
#include <fstream>
#include <curlpp/Easy.hpp>
#include <curlpp/Infos.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>
#include <iostream>
#include <sstream>
#include <future>

using namespace std;
namespace sel {
    // FIXME(TK) I do s.th. *very* unsafe and use bitlength user input directly
    // for memcpy. DO SOME SANITY CHECKS OR THIS SOFTWARE WILL BREAK AND ALLOW
    // ARBITRARY REMOTE CODE EXECUTION!
pair<FieldName, FieldEntry> parse_json_field(const ML_Field& field,
                                            const nlohmann::json& json) {
  auto logger{get_default_logger()};
  if (!(json.is_null())) {
    switch (field.type) {
      case FieldType::INTEGER: {
        const auto content{json.get<int>()};
        Bitmask temp(bitbytes(field.bitsize));
        ::memcpy(temp.data(), &content, bitbytes(field.bitsize));
        return make_pair(field.name, move(temp));
      }
      case FieldType::NUMBER: {
        const auto content{json.get<double>()};
        Bitmask temp(bitbytes(field.bitsize));
        ::memcpy(temp.data(), &content, bitbytes(field.bitsize));
        return make_pair(field.name, move(temp));
      }
      case FieldType::STRING: {
        const auto content{json.get<string>()};
        if (trim_copy(content).empty()) {
          return make_pair(field.name, nullopt);
        } else {
          const auto temp_char_array{content.c_str()};
          Bitmask temp(bitbytes(field.bitsize));
          ::memcpy(temp.data(), temp_char_array, bitbytes(field.bitsize));
          return make_pair(field.name, move(temp));
        }
      }
      case FieldType::BITMASK: {
        auto temp = json.get<string>();
        if (!trim_copy(temp).empty()) {
          auto bloom = base64_decode(temp, field.bitsize);
          if (!check_bloom_length(bloom, field.bitsize)) {
            logger->warn(
                "Bits set after bloomfilterlength. There might be a "
                "problem. Set to zero.\n");
          }
          return make_pair(field.name, move(bloom));
        } else {
          return make_pair(field.name, nullopt);
        }
      }
    }
  } else {  // field has type null
    return make_pair(field.name, nullopt);
  }
}

map<FieldName, FieldEntry> parse_json_fields(shared_ptr<const LocalConfiguration> local_config,
                                            const nlohmann::json& json) {
  map<FieldName, FieldEntry> result;
  for (auto f = json.at("fields").begin(); f != json.at("fields").end(); ++f) {
    result[f.key()] = move(parse_json_field(local_config->get_field(f.key()), *f).second);
  }
  return result;
  }

nlohmann::json read_json_from_disk(
    const experimental::filesystem::path& json_path) {
  auto logger{get_default_logger()};
  nlohmann::json content;
  if (experimental::filesystem::exists(json_path)) {
    ifstream in(json_path);
    try {
      in >> content;
    } catch (const std::ios_base::failure& e) {
      logger->error("Reading of file {} failed: {}", json_path.string(),
                    e.what());
      exit(1);
    }
    return content;
  } else {
    throw runtime_error(json_path.string() + " does not exist!");
    return content;  // To silence warning, will never be executed
  }
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
SessionResponse send_result_to_linkageservice(const SecureEpilinker::Result& share, const string& role, const shared_ptr<const LocalConfiguration>& local_config,const shared_ptr<const RemoteConfiguration>& remote_config){
  auto logger{get_default_logger()};
  nlohmann::json json_data;
  json_data["role"] = role;
  json_data["result"] = {{"match", share.match},
                          {"tentative_match", share.tmatch},
                          {"bestId", "QWxsZSBtZWluZSBFbnRjaGVuLi4u"}};
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
}  // namespace sel
