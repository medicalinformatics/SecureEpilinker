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
#include <memory>
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
    std::string url,
    Authenticator const& l_auth)
    : m_url(move(url)),
      m_local_config(move(local_conf)),
      m_local_authenticator(l_auth),
      m_logger{get_logger()} {}

DatabaseFetcher::DatabaseFetcher(
    std::shared_ptr<const LocalConfiguration> local_conf,
    std::string url,
    Authenticator const& l_auth,
    size_t page_size)
    : m_url(move(url)),
      m_local_config(move(local_conf)),
      m_local_authenticator(l_auth),
      m_page_size(page_size),
      m_logger{get_logger()} {}

DatabaseFetcher::DatabaseFetcher(
        shared_ptr<const LocalConfiguration> local_conf)
    : m_local_config(move(local_conf)),
      m_logger{get_default_logger()} {}

ServerData DatabaseFetcher::fetch_data(bool matching_mode) {
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
    save_page_data(page, matching_mode, true);
    m_next_page = page.at("_links").at("next").at("href").get<string>();
    page = get_next_page();
  }
  // Process Data from last page
  save_page_data(page, matching_mode, true);

#ifdef DEBUG_SEL_REST
  string input_string;
  for (auto& p : m_records) {
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
  if(!matching_mode){
    input_string +=
        "------------------------------\nIDs\n-----------------------------\n";
    for (const auto& m : m_ids) {
        input_string += "ID: " + m + '\n';
    }
  }
  m_logger->trace("Recieved Inputs:\n{}", input_string);
#endif

  if(matching_mode) {
    return {make_shared<VRecord>(move(m_records)), {}, m_todate, move(m_local_id), move(m_remote_id)};
  } else {
    return {make_shared<VRecord>(move(m_records)), make_shared<vector<string>>(move(m_ids)), m_todate, move(m_local_id), move(m_remote_id)};
  }
}

void DatabaseFetcher::save_page_data(const nlohmann::json& page_data, bool matching_mode, bool servermode) {
    if(servermode){
      if (!page_data.count("_links")) {
        throw runtime_error("Invalid JSON Data: missing _links section");
      }
    }
  if (!page_data.count("records")) {
    throw runtime_error("Invalid JSON Data: missing records section");
  }

  const auto& records_json = page_data["records"];
  const auto& fields = m_local_config->get_fields();
  auto temp_data = parse_json_fields_array(fields, records_json);
  append_to_map_of_vectors(temp_data, m_records);
  if(!matching_mode && servermode) {
    auto temp_ids = parse_json_id_array(records_json);
    m_ids.insert(m_ids.end(), temp_ids.begin(), temp_ids.end());
  }
}

nlohmann::json DatabaseFetcher::get_next_page() const {
  return request_page(m_next_page);
}

nlohmann::json DatabaseFetcher::request_page(const string& url) const {
  list<string> headers;
  m_logger->debug("DB request address: {}", url);
  m_logger->debug("Auth Header for DB: {}", m_local_authenticator.sign_transaction(""));
  headers.emplace_back("Authorization: "s + m_local_authenticator.sign_transaction(""));
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
