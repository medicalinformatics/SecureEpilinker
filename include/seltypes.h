/**
\file    seltypes.h
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
\brief Project specific types and enum convenience functions
*/

#ifndef SEL_SELTYPES_H
#define SEL_SELTYPES_H
#pragma once

#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include "secure_epilinker.h"

namespace sel {
class AuthenticationConfig;
// class RemoteConfiguration;

using DataField = std::variant<int, double, std::string, std::vector<uint8_t>>;
using JobId = std::string;
using RemoteId = std::string;
using ClientId = std::string;
using FieldName = std::string;
using ToDate = size_t;

enum class FieldType { BITMASK, NUMBER, STRING, INTEGER };
enum class FieldComparator { NGRAM, BINARY };
enum class AlgorithmType { EPILINK };
enum class AuthenticationType { NONE, API_KEY };
enum class JobStatus { QUEUED, RUNNING, HOLD, FAULT, DONE };

FieldType str_to_ftype(const std::string& str);
FieldComparator str_to_fcomp(const std::string& str);
AlgorithmType str_to_atype(const std::string& str);
AuthenticationType str_to_authtype(const std::string& str);
std::string js_enum_to_string(JobStatus);

struct ML_Field {
  ML_Field() = default;
  ML_Field(const std::string& n,
           double f,
           double e,
           const std::string& c,
           const std::string& t)
      : name(n), comparator(str_to_fcomp(c)), type(str_to_ftype(t)) {
    weight = std::log2((1 - e) / f);
  };
  std::string name;
  double weight;
  FieldComparator comparator;
  FieldType type;
};

struct ConnectionConfig {
  std::string url;
  std::unique_ptr<AuthenticationConfig> authentication;
};

struct CallbackConfig {
  std::string url;
  std::string token;
};

struct AlgorithmConfig {
  AlgorithmType type;
  unsigned bloom_length;
  double threshold_match;
  double threshold_non_match;
};

struct SessionResponse {
  int return_code;
  std::string body;
  std::multimap<std::string, std::string> headers;
};
struct ServerData {
  std::map<FieldName, std::vector<Bitmask>> hw_data;
  std::map<FieldName, VCircUnit> bin_data;
  std::map<FieldName, std::vector<bool>> hw_empty;
  std::map<FieldName, std::vector<bool>> bin_empty;
  std::vector<std::map<std::string, std::string>> ids;
  ToDate todate;
};
}  // namespace sel
#endif  // SEL_SELTYPES_H
