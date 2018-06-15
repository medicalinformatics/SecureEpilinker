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
#include <set>
#include <cstdint>

namespace sel {

// index type
using IndexSet = std::set<size_t>;
// weight type
using Weight = double;
using VWeight = std::vector<Weight>;
// nGram field types of which hamming weights are computed
using BitmaskUnit = uint8_t;
using Bitmask = std::vector<BitmaskUnit>;
using VBitmask = std::vector<Bitmask>;
// Circuit unit
using CircUnit = uint32_t;
using VCircUnit = std::vector<CircUnit>;
constexpr size_t BitLen = sizeof(CircUnit)*8;

struct EpilinkConfig {
  // vector of weights
  const VWeight bm_weights;
  const VWeight bin_weights;

  // exchange groups by index
  const std::vector<IndexSet> bm_exchange_groups;
  const std::vector<IndexSet> bin_exchange_groups;

  // bitlength of bitmasks
  const uint32_t size_bitmask;
  const uint32_t bytes_bitmask;
  const uint32_t size_hw;

  // thresholds
  const double threshold; // threshold for definitive match
  const double tthreshold; // threshold for tentative match

  // calculated fields for faster access
  const size_t nbm_fields, nbin_fields, nfields; // field counters
  size_t dice_prec, weight_prec; // bit precisions
  const double max_weight;

  EpilinkConfig(VWeight bm_weights, VWeight bin_weights,
      std::vector<IndexSet> bm_exchange_groups,
      std::vector<IndexSet> bin_exchange_groups,
      uint32_t size_bitmask, double threshold, double tthreshold);
  ~EpilinkConfig() = default;

  /**
   * Manually set bit precisions for dice-coefficients and weight fields
   * Sum of both must be smaller than (BitLen - ceil_log2(nfields))
   */
  void set_precisions(size_t dice_prec_, size_t weight_prec_);
};

struct EpilinkClientInput {
  // nfields vector of input record to link
  // cannot be const because ABY doens't know what const is
  const VBitmask bm_record;
  const VCircUnit bin_record;

  // corresponding empty-field-flags
  const std::vector<bool> bm_rec_empty;
  const std::vector<bool> bin_rec_empty;

  // need to know database size of remote during circuit building
  const size_t nvals;
};

struct EpilinkServerInput {
  // Outer vector by fields, inner by records!
  // Need to model like this for SIMD
  const std::vector<VBitmask> bm_database;
  const std::vector<VCircUnit> bin_database;

  // corresponding empty-field-flags
  const std::vector<std::vector<bool>> bm_db_empty;
  const std::vector<std::vector<bool>> bin_db_empty;

  const size_t nvals;
  // default constructor - checks that all records have same size
  // TODO: make it a move && constructor instead?
  EpilinkServerInput(std::vector<VBitmask> bm_database,
      std::vector<VCircUnit> bin_database,
      std::vector<std::vector<bool>> bm_db_empty,
      std::vector<std::vector<bool>> bin_db_empty);
  ~EpilinkServerInput() = default;
};

// hamming weights
CircUnit hw(const Bitmask&);
std::vector<CircUnit> hw(const std::vector<Bitmask>&);
std::vector<std::vector<CircUnit>> hw(const std::vector<std::vector<Bitmask>>&);

/*
 * Rescales the weights so that the maximum weight is the maximum element
 * of given precision bits, i.e., 0xff...
 * This should lead to the best possible precision during calculation.
 */
std::vector<CircUnit> rescale_weights(const std::vector<Weight>& weights,
    size_t prec, Weight max_weight = 0);

CircUnit rescale_weight(Weight weight, size_t prec, Weight max_weight);

} // namespace sel

#ifdef FMT_FORMAT_H_
// To use ostream&operator<< overloads
#include "fmt/ostream.h"
// Custom fmt formatters for our types
namespace fmt {

template <>
struct formatter<sel::EpilinkConfig> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const sel::EpilinkConfig& r, FormatContext &ctx) {
    return format_to(ctx.begin(),"Epilink Configuration"
      "\nBitmask size (in Bit):\t{}"
      "\nThreshold match: {}"
      "\nThreshold tentative match: {}"
      "\nNumber of bitmask field weights: {}"
      "\nNumber of binary field weights: {}",
      conf.size_bitmask, conf.threshold, conf.tthreshold,
      conf.nfields.at(FieldComparator::NGRAM),
      conf.nfields.at(FieldComparator::NGRAM)
    );
  }
};

template <>
struct formatter<sel::EpilinkClientInput> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const sel::EpilinkClientInput& input, FormatContext &ctx) {
    string returnstring{"Client Input\n-----\nBitmask Records:\n-----\n"};
    for (size_t i = 0; i != in.bm_record.size(); ++i){
      for(const auto& byte : in.bm_record[i]) {
        returnstring += to_string(byte)+' ';
      }
      returnstring += "Empty: "s+(in.bm_rec_empty[i]? "Yes\n" : "No\n");
    }
    returnstring += "-----\nBinary Records\n-----\n";
    for (size_t i = 0; i != in.bin_record.size(); ++i){
      returnstring += to_string(in.bin_record[i])+" Empty: "+(in.bin_rec_empty[i]? "Yes\n" : "No\n");
    }
    returnstring += "Number of database records: " + to_string(in.nvals);
    return format_to(ctx.begin(), returnstring);
  }
};

template <>
struct formatter<sel::EpilinkServerInput> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const sel::EpilinkServerInput& input, FormatContext &ctx) {
    return format_to(ctx.begin(), "Server Input printing not ready yet!");
  }
};

} // namespace fmt
#endif // FMT_FORMAT_H_

#endif /* end of include guard: SEL_EPILINKINPUT_H */
