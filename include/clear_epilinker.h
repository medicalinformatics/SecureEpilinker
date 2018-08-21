/**
 \file    clear_epilinker.h
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
 \brief Epilink Algorithm on clear text values / integer and exact double implementation
*/

#ifndef SEL_CLEAR_EPILINKER_H
#define SEL_CLEAR_EPILINKER_H
#pragma once

#include "fmt/format.h"
#include "circuit_config.h"

namespace sel::clear_epilink {

/**
  * Combined input of record and database to match against
  */
struct Input {
  const std::map<FieldName, FieldEntry> record;
  const std::map<FieldName, VFieldEntry> database;
  const size_t nvals;
  // Combines Epilink{Client,Server}Input
  Input(const EpilinkClientInput&, const EpilinkServerInput&);
  // Sets fields directly
  Input(const std::map<FieldName, FieldEntry>& record,
      const std::map<FieldName, VFieldEntry>& database);
};

// Result type
template<typename T>
struct Result {
  T index;
  bool match;
  bool tmatch;
  T sum_field_weights;
  T sum_weights;
};


/**
 * Implementation of the EpiLink algorithm on clear values
 * Two variations are possible:
 * 1) calc_integer() (T == CircUnit)
 *    Performs the exact same calculation as SecureEpilinker using
 *    integers (CircUnit).
 * 2) calc_exact() (T == double)
 *    Performs a high precision calculation using doubles.
 *    This implementation can be used to evaluate the errors introduced by the
 *    integer circuit implementation of EpiLink
 * There's also the general integer type template calc<T> which can be used with
 * T = uint{8,16,32,64}_t to compare circuits of different arithmetic precision
 * Don't forget to then also set bitlen to that type's bitlength when creating
 * the EpilinkConfig, and possibly adjust precisions with set_precisions().
 */
Result<CircUnit> calc_integer(const Input& input, const CircuitConfig& cfg);
Result<double> calc_exact(const Input& input, const CircuitConfig& cfg);

template<typename T> Result<T> calc(const Input& input, const CircuitConfig& cfg);

} /* end of namespace sel::clear_epilink */

// Custom fmt formatters
namespace fmt {
template <typename T>
struct formatter<sel::clear_epilink::Result<T>> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const sel::clear_epilink::Result<T>& r, FormatContext &ctx) {
    std::string type_spec;
    if constexpr (std::is_integral_v<T>) type_spec = ":x";
    return format_to(ctx.begin(),
        "best index: {}; match(/tent.)? {}/{}\n"
        "sum(field-weights): {" + type_spec + "}; "
        "sum(weights): {" + type_spec + "}; score: {}\n"
        , (uint64_t)r.index, r.match, r.tmatch
        , r.sum_field_weights, r.sum_weights,
        (((double)r.sum_field_weights)/r.sum_weights)
        );
  }
};

} // namespace fmt

#endif /* end of include guard: SEL_CLEAR_EPILINKER_H */

