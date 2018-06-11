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
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include "apikeyconfig.hpp"
#include "authenticationconfig.hpp"
#include "base64.h"
#include "epilink_input.h"
#include "fmt/format.h"
#include "localconfiguration.h"
#include "nlohmann/json.hpp"
#include "restbed"
#include "seltypes.h"
#include "util.h"

using namespace std;
namespace sel {
PollData DatabaseFetcher::fetch_data(AuthenticationConfig* l_auth) {
  fmt::print("Requesting Database\n");
  m_page = 1u;
  auto paget{
      request_page(m_url + "?pagesize=" + to_string(m_page_size), l_auth)};
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
    page = get_next_page(l_auth);
  }
  // Process Data from last page
  get_page_data(page);

  fmt::print("Recieved Inputs:\n");
  for (auto& p : m_hw_data) {
    fmt::print(
        "-------------------------------\n{}\n-------------------------------"
        "\n",
        p.first);
    for (auto& d : p.second) {
      fmt::print("Bitmask\n");
      for (const auto& byte : d)
        fmt::print("{} ", byte);
      fmt::print("\n");
    }
  }

  for (auto& p : m_bin_data) {
    fmt::print(
        "-------------------------------\n{}\n-------------------------------"
        "\n",
        p.first);
    for (auto& d : p.second) {
      fmt::print("{}\n", d);
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
  return {m_todate,         move(m_hw_data),   move(m_bin_data),
          move(m_hw_empty), move(m_bin_empty), move(m_ids)};
}

void DatabaseFetcher::get_page_data(const nlohmann::json& page_data) {
  if (!page_data.count("_links")) {
    throw runtime_error("Invalid JSON Data");
  }
  if (!page_data.count("records")) {
    throw runtime_error("Invalid JSON Data");
  }
  map<FieldName, vector<Bitmask>> temp_hw_data;
  map<FieldName, VCircUnit> temp_bin_data;
  map<FieldName, vector<bool>> temp_hw_empty;
  map<FieldName, vector<bool>> temp_bin_empty;

  for (const auto& rec : page_data["records"]) {
    if (!rec.count("fields")) {
      throw runtime_error("Invalid JSON Data");
    }
    for (auto f = rec["fields"].begin(); f != rec["fields"].end(); ++f) {
      switch (m_parent->get_field(f.key()).type) {
        case FieldType::INTEGER: {
          if (m_parent->get_field(f.key()).comparator ==
              FieldComparator::BINARY) {
            if (f->get<int>() == 0) {
              temp_bin_empty[f.key()].emplace_back(true);
            } else {
              temp_bin_empty[f.key()].emplace_back(false);
            }
            temp_bin_data[f.key()].emplace_back(
                static_cast<CircUnit>(f->get<int>()));
          } else {
            throw runtime_error(
                "NGRAM comparison not allowed for non bitmask types");
          }
          break;
        }
        case FieldType::NUMBER: {
          if (m_parent->get_field(f.key()).comparator ==
              FieldComparator::BINARY) {
            if (f->get<double>() == 0.) {
              temp_bin_empty[f.key()].emplace_back(true);
            } else {
              temp_bin_empty[f.key()].emplace_back(false);
            }
            temp_bin_data[f.key()].emplace_back(
                hash<double>{}(f->get<double>()));
          } else {
            throw runtime_error(
                "NGRAM comparison not allowed for non bitmask types");
          }
          break;
        }
        case FieldType::STRING: {
          if (m_parent->get_field(f.key()).comparator ==
              FieldComparator::BINARY) {
            if (trim_copy(f->get<string>()) == "") {
              temp_bin_empty[f.key()].emplace_back(true);
            } else {
              temp_bin_empty[f.key()].emplace_back(false);
            }
            temp_bin_data[f.key()].emplace_back(
                hash<string>{}(f->get<string>()));
          } else {
            throw runtime_error(
                "NGRAM comparison not allowed for non bitmask types");
          }
          break;
        }
        case FieldType::BITMASK: {
          auto temp = f->get<string>();
          auto bloom = base64_decode(temp);
          if (!check_bloom_length(bloom, m_parent->get_bloom_length())) {
            fmt::print(
                "Warning: Set bits after bloomfilterlength. Set to zero.\n");
          }
          if (m_parent->get_field(f.key()).comparator ==
              FieldComparator::NGRAM) {
            if (trim_copy(temp) == "") {
              temp_hw_empty[f.key()].emplace_back(true);
            } else {
              temp_hw_empty[f.key()].emplace_back(false);
            }
            temp_hw_data[f.key()].emplace_back(move(bloom));
          } else {
            throw runtime_error("BINARY comparison for bitmasks not allowed");
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
  for (auto& field : temp_hw_data) {  // Append page data to main data
    m_hw_data[field.first].insert(m_hw_data[field.first].end(),
                                  field.second.begin(), field.second.end());
  }
  for (auto& field : temp_bin_data) {  // Append page data to main data
    m_bin_data[field.first].insert(m_bin_data[field.first].end(),
                                   field.second.begin(), field.second.end());
  }
  for (auto& field : temp_hw_empty) {  // Append page empty to main data
    m_hw_empty[field.first].insert(m_hw_empty[field.first].end(),
                                   field.second.begin(), field.second.end());
  }
  for (auto& field : temp_bin_empty) {  // Append page data to main data
    m_bin_empty[field.first].insert(m_bin_empty[field.first].end(),
                                    field.second.begin(), field.second.end());
  }
}

nlohmann::json DatabaseFetcher::get_next_page(
    AuthenticationConfig* l_auth) const {
  return request_page(m_next_page, l_auth);
}

nlohmann::json DatabaseFetcher::request_page(
    const string& url,
    AuthenticationConfig* l_auth) const {
  auto request{make_shared<restbed::Request>(restbed::Uri(url))};
  request->set_method("GET");
  request->set_version(1.1);
  request->set_protocol("HTTP");
  fmt::print("DB request address: {}\n", url);
  if (l_auth->get_type() == AuthenticationType::API_KEY) {
    auto apiauth = dynamic_cast<APIKeyConfig*>(l_auth);
    request->add_header(
        "Authorization",
        string("mainzellisteApiKey apikey=\"") + apiauth->get_key() + "\"");
  }
  auto response = restbed::Http::sync(request);
  auto status = response->get_status_code();
  if (status != 400 && status != 401) {
    size_t content_length = response->get_header("Content-Length", 0);
    if (content_length) {
      restbed::Http::fetch(content_length, response);
    } else {
      fmt::print("No valid Data!");
    }

    auto body = response->get_body();
    string bodystring = string(body.begin(), body.end());
    return nlohmann::json::parse(bodystring);
  } else {
    fmt::print("Invalid Response from DB\n");
    return nlohmann::json();
  }
}
}  // namespace sel
