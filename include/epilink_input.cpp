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
#include <random>
#include <algorithm>
#include <limits>
#include <fmt/format.h>
#include "abycore/circuit/abycircuit.h"
#include "epilink_input.h"
#include "util.h"

using namespace std;
using fmt::print;

namespace sel {

EpilinkConfig::EpilinkConfig(VWeight bm_weights, VWeight bin_weights,
      vector<IndexSet> bm_exchange_groups, vector<IndexSet> bin_exchange_groups,
      uint32_t size_bitmask, double threshold, double tthreshold) :
  bm_weights{bm_weights}, bin_weights{bin_weights},
  bm_exchange_groups{bm_exchange_groups}, bin_exchange_groups{bin_exchange_groups},
  size_bitmask{size_bitmask}, bytes_bitmask{bits_in_bytes(size_bitmask)},
  size_hw{ceil_log2(size_bitmask+1)},
  threshold{threshold}, tthreshold{tthreshold},
  nbm_fields{bm_weights.size()}, nbin_fields{bin_weights.size()},
  nfields{nbm_fields + nbin_fields},
  // evenly distribute precision bits between weight and dice-coeff
  //dice_prec{(BitLen - ceil_log2(nfields))/2}, // TODO better this one and
  //custom integer division
  dice_prec{16 - 1 - ceil_log2(size_bitmask + 1)},
  //weight_prec{ceil_divide((BitLen - ceil_log2(nfields)), 2)}, // TODO ^^^
  weight_prec{BitLen - ceil_log2(nfields) - dice_prec},
  max_weight{max(
      nbm_fields ? *max_element(bm_weights.cbegin(), bm_weights.cend()) : 0.0,
      nbin_fields ? *max_element(bin_weights.cbegin(), bin_weights.cend()) :0.0
      )},
  bm_weights_r{rescale_weights(bm_weights, weight_prec, max_weight)},
  bin_weights_r{rescale_weights(bin_weights, weight_prec, max_weight)}
  {
    // We sum up nfields products of dice_prec+weight_prec values, so they must
    // fit into the total BitLen
    assert (dice_prec + weight_prec + ceil_log2(nfields) == BitLen);
#ifdef DEBUG_SEL_INPUT
    print("BitLen: {}; nfields: {}; dice precision: {}; weight precision: {}\n",
        BitLen, nfields, dice_prec, weight_prec);
#endif
  }

void EpilinkConfig::set_precisions(size_t dice_prec_, size_t weight_prec_) {
  assert (dice_prec_ + weight_prec_ + ceil_log2(nfields) <= BitLen);

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
/*
vector<Weight> gen_random_weights(const mt19937& gen, const uint32_t nfields) {
  // random weights between 1.0 and 24.0
  uniform_real_distribution<Weight> weight_dis(1.0, 24.0);
  vector<Weight> weights(nfields);
  generate(weights.begin(), weights.end(),
      [&weight_dis, &gen](){return weight_dis(gen);});
  return weights;
}

VBitmask gen_random_bm_vec(const mt19937& gen, const uint32_t size, const uint32_t bitmask_size) {
  uniform_int_distribution<BitmaskUnit> dis{};
  uint32_t bitmask_bytes = bits_in_bytes(bitmask_size);
  vector<Bitmask> ret(size);
  for (auto& bm : ret) {
    bm.resize(bitmask_bytes);
    generate(bm.begin(), bm.end(), [&gen, &dis](){ return dis(gen); });
  }
  return ret;
}

VCircUnit gen_random_bin_vec(const mt19937& gen, const uint32_t size) {
  uniform_int_distribution<CircUnit> dis{};
  vector<CircUnit> ret(size);
  generate(ret.begin(), ret.end(), [&gen, &dis](){ return dis(gen); });
  return ret;
}

EpilinkClientInput gen_random_client_input(
    const uint32_t seed, const uint32_t bitmask_size,
    const uint32_t nbm_fields, const uint32_t nbin_fields) {
  mt19937 gen(seed);

  return EpilinkClientInput{
    gen_random_weights(gen, nbm_fields),
    gen_random_weights(gen, nbin_fields),
    gen_random_bm_vec(gen, nbm_fields, bitmask_size),
    gen_random_bin_vec(gen, nbin_fields)};
}


EpilinkServerInput gen_random_server_input(
    const uint32_t seed, const uint32_t bitmask_size,
    const uint32_t nbm_fields, const uint32_t nbin_fields,
    const uint32_t nvals) {
  mt19937 gen(seed);

  // random weights
  vector<Weight> bm_weights = gen_random_weights(gen, nbm_fields);
  vector<Weight> bin_weights = gen_random_weights(gen, nbin_fields);
  // random database
  vector<vector<Bitmask>> bm_database(nbm_fields);
  for (auto& col: bm_database) {
    col = gen_random_bm_vec(gen, nvals, bitmask_size);
  }
  vector<vector<CircUnit>> bin_database(nbin_fields);
  for (auto& col: bin_database) {
    col = gen_random_bin_vec(gen, nvals);
  }

  return EpilinkServerInput{bm_weights, bin_weights,
    bm_database, bin_database};
}
*/

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
