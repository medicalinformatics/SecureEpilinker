/**
 \file    epilink_input.h
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
 \brief Encapsulation of cleartext EpiLink algorithm inputs
*/

#ifndef SEL_EPILINKINPUT_H
#define SEL_EPILINKINPUT_H
#pragma once

#include <vector>
#include <map>
#include <cstdint>
#include "seltypes.h"
#include "fmt/ostream.h"

namespace sel {

// nGram/bitmask field types of which hamming weights are computed
using BitmaskUnit = uint8_t;
using Bitmask = std::vector<BitmaskUnit>;
using VBitmask = std::vector<Bitmask>;
// How we save all input data
using FieldEntry = std::optional<Bitmask>;
using VFieldEntry = std::vector<FieldEntry>;
// Circuit unit
using CircUnit = uint32_t;
using VCircUnit = std::vector<CircUnit>;
constexpr size_t BitLen = sizeof(CircUnit)*8;

struct EpilinkConfig {
  // field descriptions
  const std::map<FieldName, ML_Field> fields;

  // exchange groups by index
  const std::vector<IndexSet> exchange_groups;

  // thresholds
  const double threshold; // threshold for definitive match
  const double tthreshold; // threshold for tentative match

  // calculated fields for faster access
  const size_t nfields; // total number of field
  size_t dice_prec, weight_prec; // bit precisions
  const Weight max_weight; // maximum weight for rescaling of weights

  EpilinkConfig(
      std::map<FieldName, ML_Field> fields,
      std::vector<IndexSet> exchange_groups,
      double threshold, double tthreshold
  );
  ~EpilinkConfig() = default;

  /**
   * Manually set bit precisions for dice-coefficients and weight fields
   * Sum of both must be smaller than (BitLen - ceil_log2(nfields))
   */
  void set_precisions(size_t dice_prec_, size_t weight_prec_);
};

struct EpilinkClientInput {
  // nfields map of input record to link
  const std::map<FieldName, FieldEntry> record;

  // need to know database size of remote during circuit building
  const size_t nvals;
};

struct EpilinkServerInput {
  // Outer vector by fields, inner by records!
  // Need to model like this for SIMD
  const std::map<FieldName, VFieldEntry> database;

  const size_t nvals;
  // default constructor - checks that all records have same size
  // TODO: make it a move && constructor instead?
  EpilinkServerInput(std::map<FieldName, VFieldEntry> database);
  ~EpilinkServerInput() = default;
};

/*
 * Rescales the weights so that the maximum weight is the maximum element
 * of given precision bits, i.e., 0xff...
 * This should lead to the best possible precision during calculation.
 */
std::vector<CircUnit> rescale_weights(const std::vector<Weight>& weights,
    size_t prec, Weight max_weight = 0);

CircUnit rescale_weight(Weight weight, size_t prec, Weight max_weight);

/**
 * bits required to store hammingweight of bitmask of given size
 */
size_t hw_size(size_t size);

} // namespace sel

std::ostream& operator<<(std::ostream& os,
    const std::pair<const sel::FieldName, sel::FieldEntry>& f);
std::ostream& operator<<(std::ostream& os, const sel::FieldEntry& val);
std::ostream& operator<<(std::ostream& os, const sel::EpilinkClientInput& in);
std::ostream& operator<<(std::ostream& os, const sel::EpilinkServerInput& in);

// Custom fmt formatters for our types
namespace fmt {

template <>
struct formatter<sel::EpilinkConfig> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const sel::EpilinkConfig& conf, FormatContext &ctx) {
    return format_to(ctx.begin(),"Epilink Configuration"
      "\nThreshold match: {}"
      "\nThreshold tentative match: {}"
      "\nNumber of fields: {}",
      conf.threshold, conf.tthreshold, conf.nfields
    );
  }
};

} // namespace fmt

#endif /* end of include guard: SEL_EPILINKINPUT_H */
