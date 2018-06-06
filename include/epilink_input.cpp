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

namespace sel {

EpilinkConfig::EpilinkConfig(v_weight_type hw_weights, v_weight_type bin_weights,
      vector<v_idx> hw_exchange_groups, vector<v_idx> bin_exchange_groups,
      uint32_t size_bitmask, double threshold, double tthreshold) :
  hw_weights{hw_weights}, bin_weights{bin_weights},
  hw_exchange_groups{hw_exchange_groups}, bin_exchange_groups{bin_exchange_groups},
  size_bitmask{size_bitmask}, bytes_bitmask{bits_in_bytes(size_bitmask)},
  size_hw{ceil_log2(size_bitmask+1)},
  threshold{threshold}, tthreshold{tthreshold},
  nhw_fields{hw_weights.size()}, nbin_fields{bin_weights.size()},
  max_weight{max( *max_element(hw_weights.cbegin(), hw_weights.cend()),
      *max_element(bin_weights.cbegin(), bin_weights.cend()) )},
  hw_weights_r{rescale_weights(hw_weights, max_weight)},
  bin_weights_r{rescale_weights(bin_weights, max_weight)}
  {}

EpilinkServerInput::EpilinkServerInput(
  vector<v_bitmask_type> hw_database, vector<v_bin_type> bin_database,
  vector<vector<bool>> hw_db_empty, vector<vector<bool>> bin_db_empty) :
  hw_database{hw_database}, bin_database{bin_database},
  hw_db_empty{hw_db_empty}, bin_db_empty{bin_db_empty},
  nvals{hw_database.at(0).size()}
{
  // check that all vectors over records have same size
  check_vectors_size(hw_database, nvals, "hw_database");
  check_vectors_size(bin_database, nvals, "bin_database");
  check_vectors_size(hw_db_empty, nvals, "hw_db_empty");
  check_vectors_size(bin_db_empty, nvals, "bin_db_empty");
}
/*
vector<weight_type> gen_random_weights(const mt19937& gen, const uint32_t nfields) {
  // random weights between 1.0 and 24.0
  uniform_real_distribution<weight_type> weight_dis(1.0, 24.0);
  vector<weight_type> weights(nfields);
  generate(weights.begin(), weights.end(),
      [&weight_dis, &gen](){return weight_dis(gen);});
  return weights;
}

v_bitmask_type gen_random_hw_vec(const mt19937& gen, const uint32_t size, const uint32_t bitmask_size) {
  uniform_int_distribution<bitmask_unit> dis{};
  uint32_t bitmask_bytes = bits_in_bytes(bitmask_size);
  vector<bitmask_type> ret(size);
  for (auto& bm : ret) {
    bm.resize(bitmask_bytes);
    generate(bm.begin(), bm.end(), [&gen, &dis](){ return dis(gen); });
  }
  return ret;
}

v_bin_type gen_random_bin_vec(const mt19937& gen, const uint32_t size) {
  uniform_int_distribution<bin_type> dis{};
  vector<bin_type> ret(size);
  generate(ret.begin(), ret.end(), [&gen, &dis](){ return dis(gen); });
  return ret;
}

EpilinkClientInput gen_random_client_input(
    const uint32_t seed, const uint32_t bitmask_size,
    const uint32_t nhw_fields, const uint32_t nbin_fields) {
  mt19937 gen(seed);

  return EpilinkClientInput{
    gen_random_weights(gen, nhw_fields),
    gen_random_weights(gen, nbin_fields),
    gen_random_hw_vec(gen, nhw_fields, bitmask_size),
    gen_random_bin_vec(gen, nbin_fields)};
}


EpilinkServerInput gen_random_server_input(
    const uint32_t seed, const uint32_t bitmask_size,
    const uint32_t nhw_fields, const uint32_t nbin_fields,
    const uint32_t nvals) {
  mt19937 gen(seed);

  // random weights
  vector<weight_type> hw_weights = gen_random_weights(gen, nhw_fields);
  vector<weight_type> bin_weights = gen_random_weights(gen, nbin_fields);
  // random database
  vector<vector<bitmask_type>> hw_database(nhw_fields);
  for (auto& col: hw_database) {
    col = gen_random_hw_vec(gen, nvals, bitmask_size);
  }
  vector<vector<bin_type>> bin_database(nbin_fields);
  for (auto& col: bin_database) {
    col = gen_random_bin_vec(gen, nvals);
  }

  return EpilinkServerInput{hw_weights, bin_weights,
    hw_database, bin_database};
}
*/

// hammingweight of bitmasks
hw_type hw(const bitmask_type& bm) {
  hw_type n = 0;
  for (auto& b : bm) {
    n += __builtin_popcount(b);
  }
  return n;
}

// hammingweight over vectors
vector<hw_type> hw(const vector<bitmask_type>& v_bm) {
  vector<hw_type> res(v_bm.size());
  transform(v_bm.cbegin(), v_bm.cend(), res.begin(),
      [] (const bitmask_type& x) -> hw_type { return hw(x); });
  return res;
}

// hammingweight over vectors of vectors
vector<vector<hw_type>> hw(const vector<vector<bitmask_type>>& v_bm) {
  vector<vector<hw_type>> res(v_bm.size());
  transform(v_bm.cbegin(), v_bm.cend(), res.begin(),
      [] (const vector<bitmask_type>& x) -> vector<hw_type> { return hw(x); });
  return res;
}

// rescale all weights to an integer, max weight being b111...
vector<hw_type> rescale_weights(const vector<weight_type>& weights,
    weight_type max_weight) {
  if (!max_weight)
    max_weight = *max_element(weights.cbegin(), weights.cend());
  constexpr hw_type max_element = numeric_limits<hw_type>::max();

  // rescale weights so that max_weight is max_element (b111...)
  vector<hw_type> ret(weights.size());
  transform(weights.cbegin(), weights.cend(), ret.begin(),
      [&max_weight] (double w) ->
      hw_type { return (w/max_weight) * max_element; });

#if DEBUG_SEL_INPUT
  cout << "Transformed weights: ";
  for (auto& w : ret)
    cout << "0x" << hex << w << ", ";
  cout << endl;
#endif

  return ret;
}

hw_type rescale_weight(weight_type weight, weight_type max_weight) {
  constexpr hw_type max_element = numeric_limits<hw_type>::max();
  return (weight/max_weight) * max_element;
}

} // namespace sel
