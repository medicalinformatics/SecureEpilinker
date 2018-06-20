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
#ifdef DEBUG_SEL_INPUT
#include <fmt/format.h>
#include <iostream>
#endif
#include "epilink_input.h"
#include "util.h"

using namespace std;
using fmt::print;

namespace sel {

constexpr auto BIN = FieldComparator::BINARY;
constexpr auto BM = FieldComparator::NGRAM;

EpilinkConfig::EpilinkConfig(
      std::map<FieldName, ML_Field> fields_,
      std::vector<IndexSet> exchange_groups_,
      uint32_t size_bitmask, double threshold, double tthreshold) :
  fields {fields_},
  exchange_groups {exchange_groups_},
  size_bitmask {size_bitmask},
  bytes_bitmask (bitbytes(size_bitmask)),
  size_hw (ceil_log2(size_bitmask+1)),
  threshold {threshold},
  tthreshold {tthreshold},
  nfields {fields.size()},
  // Evenly distribute precision bits between weight^2 and dice-coeff
  // When calculating the max of a quotient of the form fw/w, we have to compare
  // factors of the form fw*w, where the field weight fw is itself a sum of factor of a
  // weight and dice coefficient d. The denominator w is itself potentially a
  // sum of weights. So in order for the CircUnit to not overflow for a product
  // of the form sum_n(d * w) * sum_n(w), it has to hold that
  // ceil_log2(n^2) + dice_prec + 2*weight_prec <= BitLen = sizeof(CircUnit).
  //dice_prec{(BitLen - ceil_log2(nfields))/2}, // TODO better this one and
  //custom integer division
  dice_prec(16 - 1 - ceil_log2(size_bitmask + 1)),
  //weight_prec{ceil_divide((BitLen - ceil_log2(nfields)), 2)}, // TODO ^^^
  weight_prec{(BitLen - ceil_log2(nfields^2) - dice_prec)/2},
  max_weight{ *max_element(transform_map_vec(fields,
        [](auto f){return f.second.weight;})) }
  {
    // Division by 2 for weight_prec initialization could have wasted one bit
    if (dice_prec + 2*weight_prec + ceil_log2(nfields^2) < BitLen) ++dice_prec;
    assert (dice_prec + 2*weight_prec + ceil_log2(nfields^2) == BitLen);
#ifdef DEBUG_SEL_INPUT
    print("BitLen: {}; nfields: {}; dice precision: {}; weight precision: {}\n",
        BitLen, nfields, dice_prec, weight_prec);
#endif
  }

void EpilinkConfig::set_precisions(size_t dice_prec_, size_t weight_prec_) {
  if (dice_prec_ + 2*weight_prec_ + ceil_log2(nfields^2) > BitLen) {
    throw runtime_error("Given dice and weight precision would potentially let the CircUnit overflow!");
  }

  dice_prec = dice_prec_;
  weight_prec = weight_prec_;
}

EpilinkServerInput::EpilinkServerInput(
  vector<VBitmask> bm_database, vector<VCircUnit> bin_database,
  vector<vector<bool>> bm_db_empty, vector<vector<bool>> bin_db_empty) :
  bm_database{bm_database}, bin_database{bin_database},
  bm_db_empty{bm_db_empty}, bin_db_empty{bin_db_empty},
  nvals{bm_database.empty() ?
    bin_database.at(0).size() : bm_database.at(0).size()}
{
  // check that all vectors over records have same size
  check_vectors_size(bm_database, nvals, "bm_database");
  check_vectors_size(bin_database, nvals, "bin_database");
  check_vectors_size(bm_db_empty, nvals, "bm_db_empty");
  check_vectors_size(bin_db_empty, nvals, "bin_db_empty");
}

// hammingweight of bitmasks
CircUnit hw(const Bitmask& bm) {
  CircUnit n = 0;
  for (auto& b : bm) {
    n += __builtin_popcount(b);
  }
  return n;
}

// hammingweight over vectors
vector<CircUnit> hw(const vector<Bitmask>& v_bm) {
  vector<CircUnit> res(v_bm.size());
  transform(v_bm.cbegin(), v_bm.cend(), res.begin(),
      [] (const Bitmask& x) -> CircUnit { return hw(x); });
  return res;
}

// hammingweight over vectors of vectors
vector<vector<CircUnit>> hw(const vector<vector<Bitmask>>& v_bm) {
  vector<vector<CircUnit>> res(v_bm.size());
  transform(v_bm.cbegin(), v_bm.cend(), res.begin(),
      [] (const vector<Bitmask>& x) -> vector<CircUnit> { return hw(x); });
  return res;
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

} // namespace sel

std::ostream& operator<<(std::ostream& os, const sel::EpilinkClientInput& in) {
  os << "Client Input\n-----\nBitmask Records:\n-----\n";
  for (size_t i = 0; i != in.bm_record.size(); ++i){
    os << (in.bm_rec_empty[i] ? "(-) " : "(+) ");
    for (auto& b : in.bm_record[i]) os << b;
    os << '\n';
  }
  os << "-----\nBinary Records\n-----\n";
  for (size_t i = 0; i != in.bin_record.size(); ++i){
    os << (in.bin_rec_empty[i] ? "(-) " : "(+) ") << in.bin_record[i] << '\n';
  }
  return os << "Number of database records: " << in.nvals;
}
