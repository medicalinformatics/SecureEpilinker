/**
 \file    circuit_config.h
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
 \brief Circuit Configuration class and utilities
*/

#ifndef SEL_CIRCUIT_CONFIG_H
#define SEL_CIRCUIT_CONFIG_H
#pragma once

#include "epilink_input.h"

namespace sel {

using CircUnit = uint32_t;
using VCircUnit = std::vector<CircUnit>;
constexpr size_t BitLen = sizeof(CircUnit)*8;


struct CircuitConfig {
  EpilinkConfig epi;

  const bool matching_mode = false;
  const size_t bitlen = BitLen;

  // pre-calculated fields
  size_t dice_prec, weight_prec;

  CircuitConfig(const EpilinkConfig& epi,
      bool matching_mode = false, size_t bitlen = BitLen);
  ~CircuitConfig() = default;

  /**
  * Manually set bit precisions for dice-coefficients and weight fields
  * dice_prec + 2*weight_prec must be smaller than (bitlen - ceil_log2(nfields))
  */
  void set_precisions(size_t dice_prec_, size_t weight_prec_);

  /**
  * Set ideal precisions, equally distributing available bits to weight and
  * dice precision such that 2*wp + dp = bitlen - ceil_log2(n*n).
  * TODO: As of now, this cannot be used with the ABY circuit yet because we
  * use a fixed 16-bit integer division. The constructor sets the precisions
  * accordingly.
  */
  void set_ideal_precision();
};

/*
 * Rescales the weights so that the maximum weight is the maximum element
 * of given precision bits, i.e., 0xff...
 * This should lead to the best possible precision during calculation.
 */
std::vector<CircUnit> rescale_weights(const std::vector<Weight>& weights,
    size_t prec, Weight max_weight = 0);

unsigned long long rescale_weight(Weight weight, size_t prec, Weight max_weight);

/**
 * bits required to store hammingweight of bitmask of given size
 */
size_t hw_size(size_t size);

} /* END namespace sel */

// Custom fmt formatters for our types
namespace fmt {

template <>
struct formatter<sel::CircuitConfig> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const sel::CircuitConfig& conf, FormatContext &ctx) {
    return format_to(ctx.begin(),
        "CircuitConfig{{{}, mathing_mode={}, bitlen={}, "
        "precisions{{dice={}, weight={}}}}}",
        conf.epi,
        conf.matching_mode, conf.bitlen, conf.dice_prec, conf.weight_prec
    );
  }
};

} // namespace fmt

#endif /* end of include guard: SEL_CIRCUIT_CONFIG_H */
