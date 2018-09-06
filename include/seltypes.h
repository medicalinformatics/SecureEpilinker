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
#include <vector>
#include <set>
#include <cmath>
#include "fmt/format.h"

namespace sel {

using FieldName = std::string;
using IndexSet = std::set<FieldName>;
// weight type
using Weight = double;
using VWeight = std::vector<Weight>;

enum class FieldType { BITMASK, NUMBER, STRING, INTEGER };
enum class FieldComparator { DICE, BINARY };

FieldType str_to_ftype(const std::string& str);
FieldComparator str_to_fcomp(const std::string& str);
std::string ftype_to_str(const FieldType&);

struct ML_Field {
  ML_Field() = default;

  /**
   * Constructor from json
   */
  ML_Field(const std::string& name, double frequency, double error,
      const std::string& comparator, const std::string& type,
      const size_t bitsize) :
    ML_Field(name, std::log2((1-error)/frequency), str_to_fcomp(comparator),
        str_to_ftype(type), bitsize)
  {};

  /**
   * Internal constructor for testing
   */
  ML_Field(const std::string& name, const double weight,
      const FieldComparator comp, const FieldType type,
      const size_t bitsize);

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
      case sel::FieldComparator::DICE: s = "Bitmask"; break;
    }
    return format_to(ctx.begin(), s);
  }
};

template <>
struct formatter<sel::ML_Field> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const sel::ML_Field& field, FormatContext &ctx) {
    return format_to(ctx.begin(),
        "ML_Field{{name={}, weight={}, comp={}, type={}, bitsize={}}}",
        field.name, field.weight, field.comparator, ftype_to_str(field.type), field.bitsize
        );
  }
};

} // namespace fmt
#endif // SEL_SELTYPES_H
