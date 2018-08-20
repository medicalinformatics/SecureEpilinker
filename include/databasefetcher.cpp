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
#include "jsonutils.h"
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
  if (!page.count("remoteId")) {
    throw runtime_error("Invalid JSON Data: missing remoteId");
  }
  if (!page.count("localId")) {
    throw runtime_error("Invalid JSON Data: missing localId");
  }
  m_remote_id = page["remoteId"].get<RemoteId>();
  m_local_id = page["localId"].get<RemoteId>();

  for (; m_page != m_last_page; ++m_page) {
    save_page_data(page);
    m_next_page = page.at("_links").at("next").at("href").get<string>();
    page = get_next_page();
  }
  // Process Data from last page
  save_page_data(page);

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
#ifndef SEL_MATCHING_MODE
  // TODO (TK) also with mathing mode compiled, but inactive.
  input_string +=
      "------------------------------\nIDs\n-----------------------------\n";
  for (const auto& m : m_ids) {
      input_string += "ID: " + m + '\n';
  }
#endif
  m_logger->trace("Recieved Inputs:\n{}", input_string);
#endif
  return {move(m_data), move(m_ids), move(m_todate), move(m_local_id), move(m_remote_id)};
}

void DatabaseFetcher::save_page_data(const nlohmann::json& page_data) {
  if (!page_data.count("_links")) {
    throw runtime_error("Invalid JSON Data: missing _links section");
  }
  if (!page_data.count("records")) {
    throw runtime_error("Invalid JSON Data: missing records section");
  }

  const auto& records = page_data["records"];
  const auto& fields = m_local_config->get_fields();
  m_data = parse_json_fields_array(fields, records);
#ifndef SEL_MATCHING_MODE
  // TODO (TK) also with mathing mode compiled, but inactive.
  m_ids = parse_json_id_array(records);
#endif
}

nlohmann::json DatabaseFetcher::get_next_page() const {
  return request_page(m_next_page);
}

nlohmann::json DatabaseFetcher::request_page(const string& url) const {
  list<string> headers;
  m_logger->debug("DB request address: {}", url);
  if (m_local_authentication->get_type() == AuthenticationType::API_KEY) {
    auto apiauth = dynamic_cast<const APIKeyConfig*>(m_local_authentication);
    m_logger->debug("ApiKey for DB: {}", apiauth->get_key());
    headers.emplace_back("Authorization: apiKey apiKey=\""s +
                          apiauth->get_key() + "\"");
  }
  auto response{perform_get_request(url,headers, false)};
  if (response.return_code == 200) {
    if (!(response.body.empty())) {
      m_logger->trace("Response Data:\n{} - {}\n",response.return_code, response.body);
    } else {
      throw runtime_error("No valid data returned from Database");
    }
    try {
      return nlohmann::json::parse(response.body);
    } catch (const exception& e) {
      m_logger->error("Error parsing JSON from database: {}", e.what());
      return nlohmann::json();
    }
  } else {
    m_logger->error("Error getting data from data service: {} - {}", response.return_code, response.body);
    return nlohmann::json();
  }
}

}  // namespace sel
