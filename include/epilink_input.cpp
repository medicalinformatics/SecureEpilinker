/**
 \file    epilink_input.cpp
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
 \brief Encapsulation of EpiLink Algorithm Inputs
*/

#include <memory>
#include <algorithm>
#include <fmt/format.h>
#include <iostream>
#include "epilink_input.h"
#include "util.h"

using namespace std;
using fmt::print;

namespace sel {

EpilinkConfig::EpilinkConfig(
      std::map<FieldName, ML_Field> fields_,
      std::vector<IndexSet> exchange_groups_,
      double threshold, double tthreshold) :
  fields {fields_},
  exchange_groups {exchange_groups_},
  threshold {threshold},
  tthreshold {tthreshold},
  nfields {fields.size()},
  max_weight{sel::max_element(fields, [](auto f){return f.second.weight;})}
  {
    // Set dice precision according to longest bitmask
    size_t max_bm_size = 0;
    for ( const auto& f : fields ) {
      if (f.second.comparator == FieldComparator::DICE) {
        max_bm_size = max(max_bm_size, f.second.bitsize);
      }
    }
    /* Evenly distribute precision bits between weight^2 and dice-coeff
    When calculating the max of a quotient of the form fw/w, we have to compare
    factors of the form fw*w, where the field weight fw is itself a sum of factor of a
    weight and dice coefficient d. The denominator w is itself potentially a
    sum of weights. So in order for the CircUnit to not overflow for a product
    of the form sum_n(d * w) * sum_n(w), it has to hold that
    ceil_log2(n^2) + dice_prec + 2*weight_prec <= BitLen = sizeof(CircUnit).
    dice_prec = (BitLen - ceil_log2(nfields))/2; // TODO better this one and
                                                 //custom integer division
    weight_prec = ceil_divide((BitLen - ceil_log2(nfields)), 2); // TODO ^^^
    */
    dice_prec = (16 - 1 - hw_size(max_bm_size)); // -1 because of factor 2
    weight_prec = (BitLen - ceil_log2(nfields^2) - dice_prec)/2;
    // Division by 2 for weight_prec initialization could have wasted one bit
    if (dice_prec + 2*weight_prec + ceil_log2(nfields^2) < BitLen) ++dice_prec;
    assert (dice_prec + 2*weight_prec + ceil_log2(nfields^2) == BitLen);

#ifdef DEBUG_SEL_INPUT
    print("BitLen: {}; nfields: {}; dice precision: {}; weight precision: {}\n",
        BitLen, nfields, dice_prec, weight_prec);
#endif

    // Sanity checks of exchange groups
    IndexSet xgunion;
    for (const auto& group : exchange_groups) {
      const auto& f0 = fields.at(*group.cbegin());
      for (const auto& fname : group) {
        // Check if exchange groups are disjoint
        auto [_, is_new] = xgunion.insert(fname);
        if (!is_new) throw logic_error(fmt::format(
            "Exchange groups must be distinct! Field {} specified multiple times.", fname));

        const auto& f = fields.at(fname);

        // Check same comparators
        if (f.comparator != f0.comparator) {
          throw logic_error{fmt::format(
              "Cannot compare field '{}' of type {} with field '{}' of type {}",
              f.name, f.comparator, f0.name, f0.comparator)};
        }

        // Check same bitsize
        if (f.bitsize != f0.bitsize) {
          throw runtime_error{fmt::format(
              "Cannot compare field '{}' of bitsize {} with field '{}' of bitsize {}",
              f.name, f.bitsize, f0.name, f0.bitsize)};
        }
      }
    }
  }

void EpilinkConfig::set_precisions(size_t dice_prec_, size_t weight_prec_) {
  if (dice_prec_ + 2*weight_prec_ + ceil_log2(nfields^2) > BitLen) {
    throw runtime_error("Given dice and weight precision would potentially let the CircUnit overflow!");
  }

  dice_prec = dice_prec_;
  weight_prec = weight_prec_;
}

EpilinkServerInput::EpilinkServerInput(
    std::map<FieldName, VFieldEntry> database_) :
  database{database_},
  nvals{database.cbegin()->second.size()}
{
  // check that all vectors over records have same size
  for (const auto& row : database) {
    check_vector_size(row.second, nvals, "database field "s + row.first);
  }
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

#if DEBUG_SEL_INPUT
  cout << "Transformed weights: ";
  for (auto& w : ret)
    cout << "0x" << hex << w << ", ";
  cout << endl;
#endif

  return ret;
}

CircUnit rescale_weight(Weight weight, size_t prec, Weight max_weight) {
  CircUnit max_el = (1ULL << prec) - 1ULL;
  return (weight/max_weight) * max_el;
}

size_t hw_size(size_t size) {
  return ceil_log2_min1(size+1);
}

} // namespace sel

std::ostream& operator<<(std::ostream& os,
    const sel::FieldEntry& val) {
  if (val) {
    for (const auto& b : *val) os << b;
  } else {
    os << "<empty>";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os,
    const std::pair<const sel::FieldName, sel::FieldEntry>& f) {
  return os << f.first << ": " << f.second;
}

std::ostream& operator<<(std::ostream& os, const sel::EpilinkClientInput& in) {
  os << "----- Client Input -----\n";
  for (const auto& f : in.record) {
    os << f << '\n';
  }
  return os << "Number of database records: " << in.nvals;
}

std::ostream& operator<<(std::ostream& os, const sel::EpilinkServerInput& in) {
  os << "----- Server Input -----\n";
  for (const auto& fs : in.database) {
    for (size_t i = 0; i != fs.second.size(); ++i) {
      os << fs.first << '[' << i << "]: " << fs.second[i] << '\n';
    }
  }
  return os << "Number of database records: " << in.nvals;
}
