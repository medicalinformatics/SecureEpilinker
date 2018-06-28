/**
\file    localconfiguration.cpp
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
\brief Holds Information about a local connection
*/

#include "localconfiguration.h"
#include <exception>
#include <set>
#include <thread>
#include "authenticationconfig.hpp"
#include "databasefetcher.h"
#include "linkagejob.h"
#include "epilink_input.h"
#include "fmt/format.h"
#include "secure_epilinker.h"
#include "seltypes.h"
#include "util.h"

using namespace std;
namespace sel {
LocalConfiguration::LocalConfiguration(
    string&& url,
    unique_ptr<AuthenticationConfig> local_auth)
    : m_local_authentication(move(local_auth)), m_data_service_url{move(url)} {}

void LocalConfiguration::add_field(ML_Field field) {
  FieldName fieldname{field.name};
  if (field.comparator == FieldComparator::NGRAM) {
    m_hw_fields.emplace(move(fieldname), move(field));
  } else {
    m_bin_fields.emplace(move(fieldname), move(field));
  }
}

const ML_Field& LocalConfiguration::get_field(const FieldName& fieldname) const{
  if (field_hw_exists(fieldname)) {
    return cref(m_hw_fields.at(fieldname));
  } else if (field_bin_exists(fieldname)) {
    return cref(m_bin_fields.at(fieldname));
  } else {
    throw runtime_error("Field \"" + fieldname + "\" does not exist.");
  }
}

vector<double> LocalConfiguration::get_weights(
    FieldComparator comparator) const {
  vector<double> tempvec;
  if (comparator == FieldComparator::NGRAM) {
    tempvec.reserve(m_hw_fields.size());
    for (const auto& p : m_hw_fields) {
      tempvec.emplace_back(p.second.weight);
    }
  } else {
    tempvec.reserve(m_bin_fields.size());
    for (const auto& p : m_bin_fields) {
      tempvec.emplace_back(p.second.weight);
    }
  }
  return tempvec;
}

void LocalConfiguration::add_exchange_group(set<FieldName> group) {
  bool hw{false};
  bool bin{false};
  for (const auto& f : group) {
    if (!field_exists(f)) {
      throw runtime_error("Invalid Exchange Group. Field(s) does not exist!");
    }
    if (get_field(f).comparator == FieldComparator::NGRAM) {
      hw = true;
    } else {
      bin = true;
    }
  }
  if (!(!hw != !bin)) {
    throw runtime_error("Mixed Exchangegroups are not implemented");
  }
  if (hw) {
    m_hw_exchange_groups.emplace_back(move(group));
  } else {
    m_bin_exchange_groups.emplace_back(move(group));
  }
}

bool LocalConfiguration::field_exists(const FieldName& fieldname) const {
  auto hw_it = m_hw_fields.find(fieldname);
  if (hw_it != m_hw_fields.end()) {
    return true;
  }
  auto bin_it = m_bin_fields.find(fieldname);
  return (bin_it != m_bin_fields.end());
}

bool LocalConfiguration::field_hw_exists(const FieldName& fieldname) const {
  auto hw_it = m_hw_fields.find(fieldname);
  return (hw_it != m_hw_fields.end());
}

bool LocalConfiguration::field_bin_exists(const FieldName& fieldname) const {
  auto bin_it = m_bin_fields.find(fieldname);
  return (bin_it != m_bin_fields.end());
}

void LocalConfiguration::set_data_service(string&& url) {
  m_data_service_url = url;
}

void LocalConfiguration::set_local_auth(unique_ptr<AuthenticationConfig> auth) {
  m_local_authentication = move(auth);
}

vector<set<FieldName>> const& LocalConfiguration::get_exchange_group(
    FieldComparator comp) const {
  return (comp == FieldComparator::NGRAM) ? m_hw_exchange_groups
                                          : m_bin_exchange_groups;
}

vector<set<size_t>> LocalConfiguration::get_exchange_group_indices(
    FieldComparator comp) const {
  auto& exchange_groups{(comp == FieldComparator::NGRAM)
                            ? m_hw_exchange_groups
                            : m_bin_exchange_groups};
  auto& fields{(comp == FieldComparator::NGRAM) ? m_hw_fields : m_bin_fields};
  vector<set<size_t>> tempvec;
  for (auto& exchange_set : exchange_groups) {
    set<size_t> tempset;
    for (auto& exchange_field : exchange_set) {
      auto field_iterator{fields.find(exchange_field)};
      if (field_iterator == fields.end()) {
        throw runtime_error("Invalid Exchange Group!");
      }
      tempset.emplace(
          static_cast<size_t>(distance(fields.begin(), field_iterator)));
    }
    tempvec.emplace_back(move(tempset));
  }
  return tempvec;
}
}  // namespace sel
