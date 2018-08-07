/**
\file    databasefetcher.cpp
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
\brief Queries the database for patient data
*/

#include "databasefetcher.h"
#include <spdlog/spdlog.h>
#include <curlpp/Easy.hpp>
#include <curlpp/Infos.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "apikeyconfig.hpp"
#include "authenticationconfig.hpp"
#include "base64.h"
#include "epilink_input.h"
#include "fmt/format.h"
#include "localconfiguration.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "restbed"
#include "resttypes.h"
#include "restutils.h"
#include "util.h"

using namespace std;
namespace sel {
DatabaseFetcher::DatabaseFetcher(
    std::shared_ptr<const LocalConfiguration> local_conf,
    std::shared_ptr<const AlgorithmConfig> algo_conf,
    std::string url,
    AuthenticationConfig const* l_auth)
    : m_url(move(url)),
      m_local_config(move(local_conf)),
      m_algo_config(move(algo_conf)),
      m_local_authentication(l_auth),
      m_logger{get_default_logger()} {}

DatabaseFetcher::DatabaseFetcher(
    std::shared_ptr<const LocalConfiguration> local_conf,
    std::shared_ptr<const AlgorithmConfig> algo_conf,
    std::string url,
    AuthenticationConfig const* l_auth,
    size_t page_size)
    : m_url(move(url)),
      m_local_config(move(local_conf)),
      m_algo_config(move(algo_conf)),
      m_local_authentication(l_auth),
      m_page_size(page_size),
      m_logger{get_default_logger()} {}

ServerData DatabaseFetcher::fetch_data() {
  m_logger->debug("Requesting Database from {}?pageSize={}\n", m_url,
                  m_page_size);
  m_logger->info("Requesting Database");
  m_page = 1u;
  auto paget{request_page(m_url + "?pageSize=" + to_string(m_page_size))};
  auto page =
      *(paget.begin());  // FIXME(TK): JSon wird zusätzlich in Array gepack
  if (page.count("lastPageNumber")) {
    m_last_page = page.at("lastPageNumber").get<unsigned>();
  }
  if (page.count("toDate")) {
    m_todate = page.at("toDate").get<size_t>();
  }

  for (; m_page != m_last_page; ++m_page) {
    get_page_data(page);
    m_next_page = page.at("_links").at("next").at("href").get<string>();
    page = get_next_page();
  }
  // Process Data from last page
  get_page_data(page);

#ifdef DEBUG_SEL_REST
  string input_string;
  for (auto& p : m_data) {
    input_string += "-------------------------------\n" + p.first +
                    "\n-------------------------------"
                    "\n";
    for (auto& d : p.second) {
      bool empty{!d};
      input_string += "Field "s + (empty ? "" : "not ") + "empty ";
      if (!empty) {
        for (const auto& byte : d.value())
          input_string += to_string(byte) + " ";
      }
      input_string += "\n";
    }
  }
  input_string +=
      "------------------------------\nIDs\n-----------------------------\n";
  for (const auto& m : m_ids) {
    for (const auto& i : m) {
      input_string += "Type: " + i.first + ", ID: " + i.second + "; ";
    }
    input_string += "\n";
  }
  m_logger->trace("Recieved Inputs:\n{}", input_string);
#endif
  return {move(m_data), move(m_ids), move(m_todate)};
}

void DatabaseFetcher::get_page_data(const nlohmann::json& page_data) {
  if (!page_data.count("_links")) {
    throw runtime_error("Invalid JSON Data: missing _links section");
  }
  if (!page_data.count("records")) {
    throw runtime_error("Invalid JSON Data: missing records section");
  }

  for (const auto& rec : page_data["records"]) {
    if (!rec.count("fields")) {
      throw runtime_error("Invalid JSON Data: missing fields");
    }
    if (!rec.count("ids")) {
      throw runtime_error("Invalid JSON Data: missing ids");
    }
    // get data
    auto data_fields{parse_json_fields(m_local_config,rec)};
    for (auto& fields : data_fields){
      m_data[fields.first].emplace_back(move(fields.second));
    }
    // get ids
    map<string, string> tempmap;
    for (auto i = rec["ids"].begin(); i != rec["ids"].end(); ++i) {
      tempmap.emplace((*i).at("idType").get<string>(),
                      (*i).at("idString").get<string>());
    }
    m_ids.emplace_back(move(tempmap));
  }
}

nlohmann::json DatabaseFetcher::get_next_page() const {
  return request_page(m_next_page);
}

nlohmann::json DatabaseFetcher::request_page(const string& url) const {
  curlpp::Easy curl_request;
  stringstream response_stream;
  list<string> headers;
  curl_request.setOpt(new curlpp::Options::Url(url));
  curl_request.setOpt(new curlpp::Options::HttpGet(true));
  curl_request.setOpt(new curlpp::Options::SslVerifyHost(false));
  curl_request.setOpt(new curlpp::Options::SslVerifyPeer(false));
  curl_request.setOpt(new curlpp::Options::WriteStream(&response_stream));
  curl_request.setOpt(new curlpp::options::Header(0));
  m_logger->debug("DB request address: {}", url);
  if (m_local_authentication->get_type() == AuthenticationType::API_KEY) {
    auto apiauth = dynamic_cast<const APIKeyConfig*>(m_local_authentication);
    headers.emplace_back("Authorization: mainzellisteApiKey apiKey=\""s +
                         apiauth->get_key() + "\"");
  }
  curl_request.setOpt(new curlpp::Options::HttpHeader(headers));
  curl_request.perform();
  auto responsecode = curlpp::Infos::ResponseCode::get(curl_request);
  // TODO(TK): Better response handling needed
  if (responsecode != 400 && responsecode != 401) {
    if (!(response_stream.str().empty())) {
      m_logger->trace("Response Data:\n{}\n", response_stream.str());
    } else {
      throw runtime_error("No valid data returned from Database");
    }
    try {
      return nlohmann::json::parse(response_stream.str());
    } catch (const exception& e) {
      m_logger->error("Error parsing JSON from database: {}", e.what());
      return nlohmann::json();
    }
  } else {
    m_logger->error("Error parsing JSON from database");
    return nlohmann::json();
  }
}
}  // namespace sel
