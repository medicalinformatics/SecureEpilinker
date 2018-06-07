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
#include "fmt/format.h"
#include "localconfiguration.h"
#include "nlohmann/json.hpp"
#include "restbed"
#include "seltypes.h"
#include "util.h"
#include "epilink_input.h"

sel::PollData sel::DatabaseFetcher::fetch_data(AuthenticationConfig* l_auth) {
  fmt::print("Requesting Database\n");
  m_page = 1u;
  auto paget{
      request_page(m_url + "?pagesize=" + std::to_string(m_page_size), l_auth)};
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
    m_next_page = page["_links"]["next"]["href"].get<std::string>();
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
  return {m_todate,
          std::move(m_hw_data),
          std::move(m_bin_data),
          std::move(m_hw_empty),
          std::move(m_bin_empty),
          std::move(m_ids)};
}

void sel::DatabaseFetcher::get_page_data(const nlohmann::json& page_data) {
  if (!page_data.count("_links")) {
    throw std::runtime_error("Invalid JSON Data");
  }
  if (!page_data.count("records")) {
    throw std::runtime_error("Invalid JSON Data");
  }
  std::map<sel::FieldName, std::vector<sel::bitmask_type>> temp_hw_data;
  std::map<sel::FieldName, sel::v_bin_type> temp_bin_data;
  std::map<sel::FieldName, std::vector<bool>> temp_hw_empty;
  std::map<sel::FieldName, std::vector<bool>> temp_bin_empty;

  for (const auto& rec : page_data["records"]) {
    if (!rec.count("fields")) {
      throw std::runtime_error("Invalid JSON Data");
    }
    for (auto f = rec["fields"].begin(); f != rec["fields"].end(); ++f) {
      switch (m_parent->get_field(f.key()).type) {
        case sel::FieldType::INTEGER: {
          if (m_parent->get_field(f.key()).comparator ==
              sel::FieldComparator::BINARY) {
            if (f->get<int>() == 0) {
              temp_bin_empty[f.key()].emplace_back(true);
            } else {
              temp_bin_empty[f.key()].emplace_back(false);
            }
            temp_bin_data[f.key()].emplace_back(static_cast<sel::bin_type>(f->get<int>()));
          } else {
            throw std::runtime_error("NGRAM comparison not allowed for non bitmask types");
          }
          break;
        }
        case sel::FieldType::NUMBER: {
          if (m_parent->get_field(f.key()).comparator ==
              sel::FieldComparator::BINARY) {
            if (f->get<double>() == 0.) {
              temp_bin_empty[f.key()].emplace_back(true);
            } else {
              temp_bin_empty[f.key()].emplace_back(false);
            }
            temp_bin_data[f.key()].emplace_back(std::hash<double>{}(f->get<double>()));
          } else {
            throw std::runtime_error("NGRAM comparison not allowed for non bitmask types");
          }
          break;
        }
        case sel::FieldType::STRING: {
          if (m_parent->get_field(f.key()).comparator ==
              sel::FieldComparator::BINARY) {
            if (sel::trim_copy(f->get<std::string>()) == "") {
              temp_bin_empty[f.key()].emplace_back(true);
            } else {
              temp_bin_empty[f.key()].emplace_back(false);
            }
            temp_bin_data[f.key()].emplace_back(std::hash<std::string>{}(f->get<std::string>()));
          } else {
            throw std::runtime_error("NGRAM comparison not allowed for non bitmask types");
          }
          break;
        }
        case sel::FieldType::BITMASK: {
          auto temp = f->get<std::string>();
          auto bloom = base64_decode(temp);
          if (!check_bloom_length(bloom, m_parent->get_bloom_length())) {
            fmt::print(
                "Warning: Set bits after bloomfilterlength. Set to zero.\n");
          }
          if (m_parent->get_field(f.key()).comparator ==
              sel::FieldComparator::NGRAM) {
            if (sel::trim_copy(temp) == "") {
              temp_hw_empty[f.key()].emplace_back(true);
            } else {
              temp_hw_empty[f.key()].emplace_back(false);
            }
            temp_hw_data[f.key()].emplace_back(std::move(bloom));
          } else {
            throw std::runtime_error("BINARY comparison for bitmasks not allowed");
          }
        }
      }
      if (!rec.count("ids")) {
        throw std::runtime_error("Invalid JSON Data");
      }
    }
      std::map<std::string, std::string> tempmap;
      for (auto i = rec["ids"].begin(); i != rec["ids"].end(); ++i) {
        tempmap.emplace((*i)["idType"].get<std::string>(),
                        (*i)["idString"].get<std::string>());
      }
      m_ids.emplace_back(std::move(tempmap));
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

nlohmann::json sel::DatabaseFetcher::get_next_page(
    AuthenticationConfig* l_auth) const {
  return request_page(m_next_page, l_auth);
}

nlohmann::json sel::DatabaseFetcher::request_page(
    const std::string& url,
    AuthenticationConfig* l_auth) const {
  auto request{std::make_shared<restbed::Request>(restbed::Uri(url))};
  request->set_method("GET");
  request->set_version(1.1);
  request->set_protocol("HTTP");
  fmt::print("DB address: {}\n", url);
  if (l_auth->get_type() == sel::AuthenticationType::API_KEY) {
    auto apiauth = dynamic_cast<sel::APIKeyConfig*>(l_auth);
    request->add_header("Authorization",
                        std::string("mainzellisteApiKey apikey=\"") +
                            apiauth->get_key() + "\"");
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
    std::string bodystring = std::string(body.begin(), body.end());
    return nlohmann::json::parse(bodystring);
  } else {
    fmt::print("Invalid Response from DB\n");
    return nlohmann::json();
  }
}
