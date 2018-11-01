/**
 \file    circuit_config.cpp
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

#include "circuit_config.h"
#include "util.h"
#include "math.h"
#include "logger.h"

using namespace std;


namespace sel {

constexpr auto GMW = BooleanSharing::GMW;
constexpr auto YAO = BooleanSharing::YAO;

BooleanSharing other(BooleanSharing x) {
  return (x==GMW) ? YAO : GMW;
}

size_t bit_usage(const size_t dice_prec,
    const size_t weight_prec, const size_t nfields) {
  return dice_prec + 2*weight_prec + ceil_log2(nfields*nfields);
}

CircuitConfig::CircuitConfig(const EpilinkConfig& epi_,
    const std::filesystem::path& circ_dir_,
    const bool matching_mode, const size_t bitlen,
    const BooleanSharing bool_sharing_, const bool use_conversion_) :
  epi {epi_},
  circ_dir {circ_dir_},
  matching_mode {matching_mode},
  bitlen {bitlen},
  bool_sharing{bool_sharing_},
  use_conversion{use_conversion_}
{
  get_logger()->trace("Constructing CircuitConfig with {}"
      "matching_mode={}, bitlen={}, bool_sharing={}, use_conversion={}",
      epi, matching_mode, bitlen, bool_sharing, use_conversion);

#ifndef SEL_MATCHING_MODE
    if (matching_mode) throw invalid_argument(
        "This SEL is compiled without matching mode (-DSEL_MATCHING_MODE)!");
#endif

  set_ideal_precision();

  if (!std::filesystem::exists(circ_dir)) {
    throw invalid_argument(fmt::format(
          "Specified circuit dir {} doesn't exist!", circ_dir.string()));
  } else if (!std::filesystem::is_directory(circ_dir)) {
    throw invalid_argument(fmt::format(
          "Specified circuit dir {} isn't a directory!", circ_dir.string()));
  }

  get_logger()->trace("Constructed {}", *this);
}

void CircuitConfig::set_precisions(size_t dice_prec_, size_t weight_prec_) {
  get_logger()->debug("Precisions changed to dice: {}; weight: {}",
      dice_prec_, weight_prec_);

  if (bit_usage(dice_prec_, weight_prec_, epi.nfields) > bitlen) {
    throw invalid_argument("Given dice and weight precision would potentially "
        "cause overflows in current bitlen!");
  }

  dice_prec = dice_prec_;
  weight_prec = weight_prec_;
}

void CircuitConfig::set_ideal_precision() {
  size_t bits_av = bitlen - ceil_log2(epi.nfields*epi.nfields);
  size_t dice_prec = (bits_av)/3;
  size_t weight_prec = dice_prec;

  // Distribute wasted bits
  if (bits_av % 3 == 1) {
    ++dice_prec;
  } else if (bits_av % 3 == 2) {
    ++weight_prec;
  }

  set_precisions(dice_prec, weight_prec);
}

/*
 * Rescales the weights so that the maximum weight is the maximum element
 * of given precision bits, i.e., 0xff...
 * This should lead to the best possible precision during calculation.
 */
unsigned long long rescale_weight(Weight weight, size_t prec, Weight max_weight) {
  unsigned long long max_el = (1ULL << prec) - 1ULL;
  return llround((weight/max_weight) * max_el);
}

CircUnit CircuitConfig::rescaled_weight(const FieldName& name) const {
  return rescale_weight(epi.fields.at(name).weight, weight_prec, epi.max_weight);
}

CircUnit CircuitConfig::rescaled_weight(const FieldName& name1, const FieldName& name2) const {
  const auto weight = (epi.fields.at(name1).weight + epi.fields.at(name2).weight)/2.0;
  return rescale_weight(weight, weight_prec, epi.max_weight);
}

// rescale all weights to an integer, max weight being b111...
vector<CircUnit> rescale_weights(const vector<Weight>& weights,
    size_t prec, Weight max_weight) {
  if (!max_weight)
    max_weight = *max_element(weights.cbegin(), weights.cend());

  // rescale weights so that max_weight is max_element (b111...)
  vector<CircUnit> ret(weights.size());
  transform(weights.cbegin(), weights.cend(), ret.begin(),
      [&max_weight, &prec] (double w) ->
      CircUnit { return rescale_weight(w, prec, max_weight); });

  return ret;
}

size_t hw_size(size_t size) {
  return ceil_log2_min1(size+1);
}

} /* END namespace sel */
