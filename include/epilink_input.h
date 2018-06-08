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

struct EpilinkConfig {
  // vector of weights
  const VWeight hw_weights;
  const VWeight bin_weights;

  // exchange groups by index
  const std::vector<IndexSet> hw_exchange_groups;
  const std::vector<IndexSet> bin_exchange_groups;

  // bitlength of bitmasks
  const uint32_t size_bitmask;
  const uint32_t bytes_bitmask;
  const uint32_t size_hw;

  // thresholds
  double threshold; // threshold for definitive match
  double tthreshold; // threshold for tentative match

  // calculated fields for faster access
  const size_t nhw_fields, nbin_fields, nfields;
  const double max_weight;

  // must come after max_weight because of initializer list
  const VCircUnit hw_weights_r;
  const VCircUnit bin_weights_r;

  EpilinkConfig(VWeight hw_weights, VWeight bin_weights,
      std::vector<IndexSet> hw_exchange_groups,
      std::vector<IndexSet> bin_exchange_groups,
      uint32_t size_bitmask, double threshold, double tthreshold);
  ~EpilinkConfig() = default;
};

struct EpilinkClientInput {
  // nfields vector of input record to link
  // cannot be const because ABY doens't know what const is
  const VBitmask hw_record;
  const VCircUnit bin_record;

  // corresponding empty-field-flags
  const std::vector<bool> hw_rec_empty;
  const std::vector<bool> bin_rec_empty;

  // need to know database size of remote during circuit building
  const size_t nvals;
};

struct EpilinkServerInput {
  // Outer vector by fields, inner by records!
  // Need to model like this for SIMD
  const std::vector<VBitmask> hw_database;
  const std::vector<VCircUnit> bin_database;

  // corresponding empty-field-flags
  const std::vector<std::vector<bool>> hw_db_empty;
  const std::vector<std::vector<bool>> bin_db_empty;

  const size_t nvals;
  // default constructor - checks that all records have same size
  // TODO: make it a move && constructor instead?
  EpilinkServerInput(std::vector<VBitmask> hw_database,
      std::vector<VCircUnit> bin_database,
      std::vector<std::vector<bool>> hw_db_empty,
      std::vector<std::vector<bool>> bin_db_empty);
  ~EpilinkServerInput() = default;
};

// hamming weights
CircUnit hw(const Bitmask&);
std::vector<CircUnit> hw(const std::vector<Bitmask>&);
std::vector<std::vector<CircUnit>> hw(const std::vector<std::vector<Bitmask>>&);

/*
 * Rescales the weights so that the maximum weight is the maximum element
 * of CircUnit, i.e., 0xff...
 * This should lead to the best possible precision during calculation.
 */
std::vector<CircUnit> rescale_weights(const std::vector<Weight>& weights,
    Weight max_weight = 0);

CircUnit rescale_weight(Weight weight, Weight max_weight);

// random data generator
// TODO adopt new API
/*
EpilinkClientInput gen_random_client_input(
  const uint32_t seed, const uint32_t bitmask_size,
  const uint32_t nhw_fields, const uint32_t nbin_fields);
EpilinkServerInput gen_random_server_input(
    const uint32_t seed, const uint32_t bitmask_size,
    const uint32_t nhw_fields, const uint32_t nbin_fields,
    const uint32_t nvals);
*/

} // namespace sel
#endif /* end of include guard: SEL_EPILINKINPUT_H */
