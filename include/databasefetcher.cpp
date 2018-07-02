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
#include <curlpp/Easy.hpp>
#include <curlpp/Infos.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include "apikeyconfig.hpp"
#include "authenticationconfig.hpp"
#include "base64.h"
#include "epilink_input.h"
#include "fmt/format.h"
#include "localconfiguration.h"
#include "nlohmann/json.hpp"
#include "restbed"
#include "util.h"
#include "resttypes.h"

using namespace std;
namespace sel {
ServerData DatabaseFetcher::fetch_data() {
  fmt::print("Requesting Database\n");
  m_page = 1u;
  auto paget{request_page(m_url + "?pagesize=" + to_string(m_page_size))};
  auto page =
      *(paget.begin());  // FIXME(TK): JSon wird zusätzlich in Array gepack
  if (page.count("lastPageNumber")) {
    m_last_page = page["lastPageNumber"].get<unsigned>();
  }
  if (page.count("todate")) {
    m_todate = page["todate"].get<size_t>();
  }

  for (; m_page != m_last_page; ++m_page) {
    get_page_data(page);
    m_next_page = page["_links"]["next"]["href"].get<string>();
    page = get_next_page();
  }
  // Process Data from last page
  get_page_data(page);

  fmt::print("Recieved Inputs:\n");
  for (auto& p : m_data) {
    fmt::print(
        "-------------------------------\n{}\n-------------------------------"
        "\n",
        p.first);
    for (auto& d : p.second) {
      bool empty{d};
      fmt::print("Field {}empty", empty?"":"not ");
      if(!empty){
      for (const auto& byte : d.value())
        fmt::print("{} ", byte);
      fmt::print("\n");
      }
    }
  }

  fmt::print(
      "------------------------------\nIDs\n-----------------------------\n");
  for (const auto& m : m_ids) {
    for (const auto& i : m) {
      fmt::print("Type: {}, ID: {}; ", i.first, i.second);
    }
    fmt::print("\n");
  }
  return {move(m_data), move(m_ids), move(m_todate)};
}

void DatabaseFetcher::get_page_data(const nlohmann::json& page_data) {
  if (!page_data.count("_links")) {
    throw runtime_error("Invalid JSON Data");
  }
  if (!page_data.count("records")) {
    throw runtime_error("Invalid JSON Data");
  }
  map<FieldName, VFieldEntry> temp_data;

  for (const auto& rec : page_data["records"]) {
    if (!rec.count("fields")) {
      throw runtime_error("Invalid JSON Data");
    }
    for (auto f = rec["fields"].begin(); f != rec["fields"].end(); ++f) {
      switch (m_local_config->get_field(f.key()).type) {
        case FieldType::INTEGER: {
            const auto content{f->get<int>()};
            if (content == 0) {
              temp_data[f.key()].emplace_back(nullopt);
            } else {
            Bitmask temp(sizeof(content));
            ::memcpy(temp.data(), &content, sizeof(content));
            temp_data[f.key()].emplace_back(move(temp));
            }
          break;
        }
        case FieldType::NUMBER: {
          const auto content{f->get<double>()};
            if (content == 0.) {
              temp_data[f.key()].emplace_back(nullopt);
            } else {
            Bitmask temp(sizeof(content));
            ::memcpy(temp.data(), &content, sizeof(content));
            temp_data[f.key()].emplace_back(move(temp));
            }
          break;
        }
        case FieldType::STRING: {
          const auto content{f->get<string>()};
            if (trim_copy(content).empty()) {
              temp_data[f.key()].emplace_back(nullopt);
            } else {
              const auto temp_char_array{content.c_str()};
              Bitmask temp(sizeof(temp_char_array));
            ::memcpy(temp.data(), &temp_char_array, sizeof(temp_char_array));
            temp_data[f.key()].emplace_back(move(temp));
            }
          break;
        }
        case FieldType::BITMASK: {
          auto temp = f->get<string>();
          auto bloom = base64_decode(temp);
          if (!check_bloom_length(bloom, m_algo_config->bloom_length)) {
            fmt::print(
                "Warning: Set bits after bloomfilterlength. Set to zero.\n");
          }
            bool bloomempty{true};
            for (const auto& byte : bloom) {
              if (bool byte_empty = (byte == 0x00); !byte_empty) {
                bloomempty = false;
                break;
              }
            }
            if (bloomempty) {
              temp_data[f.key()].emplace_back(nullopt);
            } else {
              temp_data[f.key()].emplace_back(move(bloom));
            }
        }
      }
      if (!rec.count("ids")) {
        throw runtime_error("Invalid JSON Data");
      }
    }
    map<string, string> tempmap;
    for (auto i = rec["ids"].begin(); i != rec["ids"].end(); ++i) {
      tempmap.emplace((*i)["idType"].get<string>(),
                      (*i)["idString"].get<string>());
    }
    m_ids.emplace_back(move(tempmap));
  }
  for (auto& field : temp_data) {  // Append page data to main data
    m_data[field.first].insert(m_data[field.first].end(),
                                  field.second.begin(), field.second.end());
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
  fmt::print("DB request address: {}\n", url);
  if (m_local_authentication->get_type() == AuthenticationType::API_KEY) {
    auto apiauth = dynamic_cast<const APIKeyConfig*>(m_local_authentication);
    headers.emplace_back("Authorization: mainzellisteApiKey apikey=\""s +
                         apiauth->get_key() + "\"");
  }
  curl_request.setOpt(new curlpp::Options::HttpHeader(headers));
  curl_request.perform();
  auto responsecode = curlpp::Infos::ResponseCode::get(curl_request);
  if (responsecode != 400 && responsecode != 401) {
    if (!(response_stream.str().empty())) {
      fmt::print("Response Data:\n{}\n", response_stream.str());
    } else {
      fmt::print("No valid Data!\n");
    }
    return nlohmann::json::parse(response_stream.str());
  } else {
    fmt::print("Invalid Response from DB\n");
    return nlohmann::json();
  }
}
}  // namespace sel
