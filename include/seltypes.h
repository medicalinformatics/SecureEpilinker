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

#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <set>
#include <cmath>
#include "fmt/format.h"

namespace sel {
class AuthenticationConfig;
// class RemoteConfiguration;

using DataField = std::variant<int, double, std::string, std::vector<uint8_t>>;
using FieldName = std::string;
using IndexSet = std::set<FieldName>;
// weight type
using Weight = double;
using VWeight = std::vector<Weight>;

enum class FieldType { BITMASK, NUMBER, STRING, INTEGER };
enum class FieldComparator { NGRAM, BINARY };

FieldType str_to_ftype(const std::string& str);
FieldComparator str_to_fcomp(const std::string& str);

struct ML_Field {
  ML_Field() = default;
  /**
   * Constructor from json
   */
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
  /**
   * Internal constructor for testing
   */
  ML_Field(const std::string& name, const double weight,
      const FieldComparator comp, const FieldType type,
      const size_t bitsize) :
    name{name}, weight{weight},
    comparator{comp}, type{type},
    bitsize{bitsize} {};

  std::string name;
  Weight weight;
  FieldComparator comparator;
  FieldType type;
  size_t bitsize;
};
} // namespace sel

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
#endif // SEL_SELTYPES_H
