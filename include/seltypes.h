/**
\file    seltypes.h
\author  Tobias Kussel <kussel@cbs.tu-darmstadt.de>
\author  Sebastian Stammler <sebastian.stammler@cysec.de>
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

#include <map>
#include <string>
#include <memory>
#include <vector>
#include <variant>
#include <set>
#include <cmath>

namespace sel{
class AuthenticationConfig;
//class RemoteConfiguration;


using DataField = std::variant<int, double, std::string, std::vector<uint8_t>>;
using FieldName = std::string;
using IndexSet = std::set<FieldName>;
// weight type
using Weight = double;
using VWeight = std::vector<Weight>;

// TODO#22 move REST stuff to resttypes.h
using JobId = std::string;
using RemoteId = std::string;

enum class FieldType {
  BITMASK,
  NUMBER,
  STRING,
  INTEGER
};
enum class FieldComparator {
  NGRAM,
  BINARY
};
enum class AlgorithmType {
  EPILINK
};
enum class AuthenticationType {
  NONE,
  API_KEY
};
enum class JobStatus {
  QUEUED,
  RUNNING,
  HOLD,
  FAULT,
  DONE
};

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
           const std::string& t,
           const size_t b)
      : name{n},
        weight{std::log2((1-e)/f)},
        comparator{str_to_fcomp(c)},
        type{str_to_ftype(t)},
        bitsize{b}
        {};
  std::string name;
  Weight weight;
  FieldComparator comparator;
  FieldType type;
  size_t bitsize;
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
} // namespace sel

#ifdef FMT_FORMAT_H_
// Custom fmt formatters for our types
namespace fmt {
template <>
struct formatter<sel::FieldComparator> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const sel::FieldComparator& c, FormatContext &ctx) {
    std::string s;
    switch(c) {
      case sel::FieldComparator::BINARY: s = "Binary"; break;
      case sel::FieldComparator::NGRAM: s = "Bitmask"; break;
    }
    return format_to(ctx.begin(), s);
  }
};
} // namespace fmt
#endif // FMT_FORMAT_H_

#endif // SEL_SELTYPES_H
