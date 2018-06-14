
/**
 \file    secure_epilinker.cpp
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
 \brief Encapsulation class for the secure epilink s2PC process
*/

#include <stdexcept>
#include <algorithm>
#include <fmt/format.h>
#include "secure_epilinker.h"
#include "epilink_input.h"
#include "math.h"
#include "util.h"
#include "aby/Share.h"
#include "aby/gadgets.h"

using namespace std;
using fmt::format;

namespace sel {

/***************** Circuit gadgets *******************/
BoolShare compare_hw(const BoolShare& x, const BoolShare& y,
    const BoolShare& hw_x, const BoolShare& hw_y, size_t prec) {
  // calc HW of AND and bit-shift to multiply with 2 and get dice precision
  // for integer-divivision
  BoolShare hw_and = hammingweight(x & y);
  BoolShare hw_and_shifted = hw_and << (prec + 1);
#ifdef DEBUG_SEL_CIRCUIT
  print_share(hw_and, "hw_and");
  print_share(hw_and_shifted, "hw_and_shifted");
#endif

  // Add single HWs
  BoolShare hw_plus = hw_x + hw_y;
#ifdef DEBUG_SEL_CIRCUIT
  print_share(hw_plus, "hw_plus");
#endif

  // int-divide
  BoolShare hw_div = apply_file_binary(hw_and_shifted, hw_plus, 16, 16, "circ/int_div_16.aby");
#ifdef DEBUG_SEL_CIRCUIT
  print_share(hw_div, "hw_div");
#endif
  return hw_div;
}

/*
 * Binary-compares two shares
 */
BoolShare compare_bin(const BoolShare& x, const BoolShare& y) {
  BoolShare cmp{(x == y)};
#ifdef DEBUG_SEL_CIRCUIT
  print_share(cmp, "compare_bin");
#endif
  return cmp;
}

/******************** Circuit Builder ********************/
class SecureEpilinker::SELCircuit {
public:
  SELCircuit(EpilinkConfig cfg,
    BooleanCircuit* bcirc, BooleanCircuit* ccirc, ArithmeticCircuit* acirc) : cfg{cfg},
    bcirc{bcirc}, ccirc{ccirc}, acirc{acirc},
    hw_client{cfg.nhw_fields}, hw_client_hw{cfg.nhw_fields}, hw_client_empty{cfg.nhw_fields},
    bin_client{cfg.nbin_fields}, bin_client_empty{cfg.nbin_fields},
    hw_server{cfg.nhw_fields}, hw_server_hw{cfg.nhw_fields}, hw_server_empty{cfg.nhw_fields},
    bin_server{cfg.nbin_fields}, bin_server_empty{cfg.nbin_fields}
  {}

  /**
   * Return type of weight_compare_*()
   * fw - field weight = weight * comparison * empty-flags
   * w - weight for weight sum = weight * empyt-flags
   */
  struct FieldWeight { ArithShare fw, w; };
  using Comparator = function<BoolShare (const BoolShare&, const BoolShare&)>;

  void set_client_input(const EpilinkClientInput& input) {
    set_constants(input.nvals);
    // First create the client and input shares, which differ for client and
    // server run. Then pass to the joint routine.
    for (size_t i = 0; i != cfg.nhw_fields; ++i) {
      // bitmask records are saved as vector<uint8_t>, so need to access raw data
      check_vector_size(input.hw_record[i], cfg.bytes_bitmask, "rec bitmask byte vector");
      hw_client[i] = BoolShare(bcirc,
          repeat_vec(input.hw_record[i], nvals).data(),
          cfg.size_bitmask, CLIENT, nvals);

      hw_client_hw[i] = BoolShare(bcirc,
          vector<CircUnit>(nvals, hw(input.hw_record[i])).data(),
          cfg.size_hw, CLIENT, nvals);

      // need to convert bool into uint8_t array
      hw_client_empty[i] = BoolShare(bcirc,
          vector<uint8_t>(nvals, input.hw_rec_empty[i]).data(),
          1, CLIENT, nvals);

      hw_client_delta[i] = ArithShare(acirc,
          vector<CircUnit>(nvals, !input.hw_rec_empty[i]).data(),
          BitLen, CLIENT, nvals);

      hw_server[i] = BoolShare(bcirc, cfg.size_bitmask, nvals); //dummy

      hw_server_hw[i] = BoolShare(bcirc, cfg.size_hw, nvals); //dummy

      hw_server_empty[i] = BoolShare(bcirc, 1, nvals); // dummy

      hw_server_delta[i] = ArithShare(acirc, BitLen, nvals); // dummy
#ifdef DEBUG_SEL_CIRCUIT
      print_share(hw_client[i], format("hw_client[{}]", i));
      print_share(hw_client_hw[i], format("hw_client_hw[{}]", i));
      print_share(hw_client_empty[i], format("hw_client_empty[{}]", i));
      print_share(hw_server[i], format("hw_server[{}]", i));
      print_share(hw_server_hw[i], format("hw_server_hw[{}]", i));
      print_share(hw_server_empty[i], format("hw_server_empty[{}]", i));
#endif
    }
    for (size_t i = 0; i != cfg.nbin_fields; ++i) {
      bin_client[i] = BoolShare(bcirc,
          vector<CircUnit>(nvals, input.bin_record[i]).data(),
          BitLen, CLIENT, nvals);

      bin_client_empty[i] = BoolShare(bcirc,
          vector<uint8_t>(nvals, input.bin_rec_empty[i]).data(),
          1, CLIENT, nvals);

      bin_client_delta[i] = ArithShare(acirc,
          vector<CircUnit>(nvals, !input.bin_rec_empty[i]).data(),
          BitLen, CLIENT, nvals);

      bin_server[i] = BoolShare(bcirc, BitLen, nvals); //dummy

      bin_server_empty[i] = BoolShare(bcirc, 1, nvals); // dummy

      bin_server_delta[i] = ArithShare(acirc, BitLen, nvals); // dummy
#ifdef DEBUG_SEL_CIRCUIT
      print_share(bin_client[i], format("bin_client[{}]", i));
      print_share(bin_client_empty[i], format("bin_client_empty[{}]", i));
      print_share(bin_server[i], format("bin_server[{}]", i));
      print_share(bin_server_empty[i], format("bin_server_empty[{}]", i));
#endif
    }
    is_input_set = true;
  }

  void set_server_input(const EpilinkServerInput& input) {
    set_constants(input.nvals);

    // First create the server and input shares, which differ for client and
    // server run. Then pass to the joint routine.
    // concatenated hw input garbage - needs to be in scope until circuit has executed
    vector<CircUnit> hw_db_delta(nvals);
    for (size_t i = 0; i != cfg.nhw_fields; ++i) {
      hw_client[i] = BoolShare(bcirc, cfg.size_bitmask, nvals); //dummy

      hw_client_hw[i] = BoolShare(bcirc, cfg.size_hw, nvals); //dummy

      hw_client_empty[i] = BoolShare(bcirc, 1, nvals); // dummy

      hw_client_delta[i] = ArithShare(acirc, BitLen, nvals); // dummy

      check_vectors_size(input.hw_database[i], cfg.bytes_bitmask, " db bitmask byte vectors");
      hw_server[i] = BoolShare(bcirc,
          concat_vec(input.hw_database[i]).data(),
          cfg.size_bitmask, SERVER, nvals);

      hw_server_hw[i] = BoolShare(bcirc,
          hw(input.hw_database[i]).data(),
          cfg.size_hw, SERVER, nvals);

      // need to convert bool array into uint8_t array
      hw_server_empty[i] = BoolShare(bcirc,
         vector<uint8_t>(input.hw_db_empty[i].cbegin(),
           input.hw_db_empty[i].cend()).data(),
         1, SERVER, nvals);

      for (size_t j=0; j!=nvals; ++j) hw_db_delta[j] = !input.hw_db_empty[i][j];
      hw_server_delta[i] = ArithShare(acirc, hw_db_delta.data(),
          BitLen, SERVER, nvals);
#ifdef DEBUG_SEL_CIRCUIT
      print_share(hw_client[i], format("hw_client[{}]", i));
      print_share(hw_client_hw[i], format("hw_client_hw[{}]", i));
      print_share(hw_client_empty[i], format("hw_client_empty[{}]", i));
      print_share(hw_server[i], format("hw_server[{}]", i));
      print_share(hw_server_hw[i], format("hw_server_hw[{}]", i));
      print_share(hw_server_empty[i], format("hw_server_empty[{}]", i));
#endif
    }

    vector<CircUnit> bin_db_delta(nvals);
    for (size_t i = 0; i != cfg.nbin_fields; ++i) {
      bin_client[i] = BoolShare(bcirc, BitLen, nvals); //dummy

      bin_client_empty[i] = BoolShare(bcirc, 1, nvals); // dummy

      bin_client_delta[i] = ArithShare(acirc, BitLen, nvals); // dummy

      bin_server[i] = BoolShare(bcirc,
          const_cast<CircUnit*>(input.bin_database[i].data()),
          BitLen, SERVER, nvals);

      bin_server_empty[i] = BoolShare(bcirc,
         vector<uint8_t>(input.bin_db_empty[i].cbegin(),
           input.bin_db_empty[i].cend()).data(),
          1, SERVER, nvals);

      for (size_t j=0; j!=nvals; ++j) bin_db_delta[j] = !input.bin_db_empty[i][j];
      bin_server_delta[i] = ArithShare(acirc, bin_db_delta.data(),
          BitLen, SERVER, nvals);
#ifdef DEBUG_SEL_CIRCUIT
      print_share(bin_client[i], format("bin_client[{}]", i));
      print_share(bin_client_empty[i], format("bin_client_empty[{}]", i));
      print_share(bin_server[i], format("bin_server[{}]", i));
      print_share(bin_server_empty[i], format("bin_server_empty[{}]", i));
#endif
    }
    is_input_set = true;
  }

#ifdef DEBUG_SEL_CIRCUIT
  /**
   * Debugging test_both_inputs() to test exactly mirrored inputs.
   */
  void set_both_inputs(const EpilinkClientInput& in_client, const EpilinkServerInput& in_server) {
    set_constants(in_client.nvals);
    // First create the client and input shares, which differ for client and
    // server run. Then pass to the joint routine.
    // concatenated hw input garbage - needs to be in scope until circuit has executed
    for (size_t i = 0; i != cfg.nhw_fields; ++i) {
      // bitmask records are saved as vector<uint8_t>, so need to access raw data
      check_vector_size(in_client.hw_record[i], cfg.bytes_bitmask, "rec bitmask byte vector");
      hw_client[i] = BoolShare(bcirc,
          repeat_vec(in_client.hw_record[i], nvals).data(),
          cfg.size_bitmask, CLIENT, nvals);

      hw_client_hw[i] = BoolShare(bcirc,
          vector<CircUnit>(nvals, hw(in_client.hw_record[i])).data(),
          cfg.size_hw, CLIENT, nvals);

      // need to convert bool into uint8_t array
      hw_client_empty[i] = BoolShare(bcirc,
          vector<uint8_t>(nvals, in_client.hw_rec_empty[i]).data(),
          1, CLIENT, nvals);

      check_vectors_size(in_server.hw_database[i], cfg.bytes_bitmask, " db bitmask byte vectors");
      hw_server[i] = BoolShare(bcirc,
          concat_vec(in_server.hw_database[i]).data(),
          cfg.size_bitmask, SERVER, nvals);

      hw_server_hw[i] = BoolShare(bcirc,
          hw(in_server.hw_database[i]).data(),
          cfg.size_hw, SERVER, nvals);

      // need to convert bool array into uint8_t array
      hw_server_empty[i] = BoolShare(bcirc,
         vector<uint8_t>(in_server.hw_db_empty[i].cbegin(),
           in_server.hw_db_empty[i].cend()).data(),
         1, SERVER, nvals);

      print_share(hw_client[i], format("hw_client[{}]", i));
      print_share(hw_client_hw[i], format("hw_client_hw[{}]", i));
      print_share(hw_client_empty[i], format("hw_client_empty[{}]", i));
      print_share(hw_server[i], format("hw_server[{}]", i));
      print_share(hw_server_hw[i], format("hw_server_hw[{}]", i));
      print_share(hw_server_empty[i], format("hw_server_empty[{}]", i));
    }

    for (size_t i = 0; i != cfg.nbin_fields; ++i) {
      bin_client[i] = BoolShare(bcirc,
          vector<CircUnit>(nvals, in_client.bin_record[i]).data(),
          BitLen, CLIENT, nvals);

      bin_client_empty[i] = BoolShare(bcirc,
          vector<uint8_t>(nvals, in_client.bin_rec_empty[i]).data(),
          1, CLIENT, nvals);

      bin_server[i] = BoolShare(bcirc,
          const_cast<CircUnit*>(in_server.bin_database[i].data()),
          BitLen, SERVER, nvals);

      bin_server_empty[i] = BoolShare(bcirc,
         vector<uint8_t>(in_server.bin_db_empty[i].cbegin(),
           in_server.bin_db_empty[i].cend()).data(),
          1, SERVER, nvals);

      print_share(bin_client[i], format("bin_client[{}]", i));
      print_share(bin_client_empty[i], format("bin_client_empty[{}]", i));
      print_share(bin_server[i], format("bin_server[{}]", i));
      print_share(bin_server_empty[i], format("bin_server_empty[{}]", i));
    }

    is_input_set = true;
  }
#endif

  struct ResultShares {
    OutShare index, match, tmatch;
#ifdef DEBUG_SEL_RESULT
    OutShare score_numerator, score_denominator;
#endif
  };
  /*
  * Builds the shared component of the circuit after initial input shares of
  * client and server have been created.
  */
  ResultShares build_circuit() {
    // Where we store all group and individual comparison weights as ariths
    vector<ArithShare> a_field_weights;
    vector<BoolShare> b_field_weights;
    //field_weights.reserve(cfg.hw_exchange_groups.size() + cfg.bin_exchange_groups.size());

    // 1. HW fields
    // 1.1 For all HW exchange groups, find the permutation with the highest weight
    set<size_t> no_x_group_hw; // where we save remaining indices
    // fill with ascending indices, remove used ones later
    for (size_t i = 0; i != cfg.nhw_fields; ++i)
      no_x_group_hw.insert(no_x_group_hw.end(), i);
    // for each group, store the best permutation's weight into field_weights
    for (auto& group : cfg.hw_exchange_groups) {
      // add this group's weight to vector
      BoolShare group_weight{best_group_weight_hw(group)};
      b_field_weights.emplace_back(group_weight);
      // remove all indices that were covered by this index group
      for (const size_t& i : group) no_x_group_hw.erase(i);
    }
    // 1.2 Remaining HW indices
    for (size_t i : no_x_group_hw) {
      a_field_weights.emplace_back(weight_compare_hw(i, i));
    }

    // 2. Binary fields
    // TODO we ignore exchange groups for now, as there are none in
    // Mainzelliste's current config
    for (size_t i{0}; i != cfg.nbin_fields; ++i) {
      b_field_weights.emplace_back(weight_compare_bin(i, i));
    }

    // 3. Sum up all field weights.
    // best_group_weight_*() and weight_compare_bin() return BoolShares and we
    // don't need arithmetic representation afterwards, so we sum those in the
    // boolean circuit.
    if (!a_field_weights.empty())
      b_field_weights.emplace_back(to_bool(sum(a_field_weights)));
    BoolShare sum_field_weights{sum(b_field_weights)};
#ifdef DEBUG_SEL_CIRCUIT
    print_share(sum_field_weights, "sum_field_weights");
#endif

    // 4. Determine index of max score of all nvals calculations
    BoolShare max_idx{const_idx};
    assert((bool)const_idx); // check that it was copied, not moved
    split_select_target(sum_field_weights, max_idx,
        (BinaryOp_BoolShare) [](auto a, auto b) {return a>b;});

    // 5. Left side: sum up all weights and multiply with threshold
    //  Since weights and threshold are public inputs, this can be computed
    //  locally. This is done in set_constants().
    //
    // 6. Set two comparison bits, whether > (tentative) threshold
    BoolShare match = const_w_threshold < sum_field_weights;
    BoolShare tmatch = const_w_tthreshold < sum_field_weights;
#ifdef DEBUG_SEL_CIRCUIT
    print_share(sum_field_weights, "best score");
    print_share(max_idx, "index of best score");
    print_share(match, "match?");
    print_share(tmatch, "tentative match?");
#endif

#ifdef DEBUG_SEL_RESULT
      //TODO Actually return W = sum of non-empty field weights
    return {out(max_idx, ALL), out(match, ALL), out(tmatch, ALL),
      out(sum_field_weights, ALL),
      out(constant(bcirc, W<<cfg.dice_prec, BitLen), ALL)};
#else
    return {out_shared(max_idx), out_shared(match), out_shared(tmatch)};
#endif
  }

private:
  // Epilink config
  const EpilinkConfig cfg;
  // Circuits
  BooleanCircuit* bcirc; // boolean circuit for boolean parts
  BooleanCircuit* ccirc; // intermediate conversion circuit
  ArithmeticCircuit* acirc;
  // Input shares
  struct ValueShare {
    BoolShare val; // value as bool
    ArithShare delta; // 1 if non-empty
    BoolShare hw; // precomputed HW of val - not used for bin
  };
  struct InputShares {
    ValueShare client, server;
  };
  InputShares bm, bin;
  vector<BoolShare>
    hw_client, hw_client_hw, hw_client_empty,
    hw_server, hw_server_hw, hw_server_empty,
    bin_client, bin_client_empty,
    bin_server, bin_server_empty;
  vector<ArithShare>
    hw_client_delta, hw_server_delta,
    bin_client_delta, bin_server_delta;
  // Constant shares
  BoolShare const_zero, const_idx;
  // Left side of inequality: T * sum(weights)
  BoolShare const_w_threshold, const_w_tthreshold;

#ifdef DEBUG_SEL_RESULT
  CircUnit W{0};
#endif

  // state variables
  uint32_t nvals{0};
  bool is_input_set{false};

  // Dynamic converters, dependent on main bool sharing
  BoolShare to_bool(const ArithShare& s) {
    return (bcirc->GetContext() == S_YAO) ? a2y(bcirc, s) : a2b(bcirc, ccirc, s);
  }
  ArithShare to_arith(const BoolShare& s_) {
    BoolShare s{s_.zeropad(BitLen)}; // fix for aby issue #46
    return (bcirc->GetContext() == S_YAO) ? y2a(acirc, ccirc, s) : b2a(acirc, s);
  }

  void set_constants(uint32_t nvals) {
    this->nvals = nvals;
    // create constants with nvals
    const_zero = constant_simd(bcirc, 0, 1, nvals);
    // build constant index vector
    vector<BoolShare> numbers;
    numbers.reserve(nvals);
    for (CircUnit i = 0; i != nvals; ++i) {
      // TODO Make true SIMD constants available in ABY and implement offline
      // AND with constant
      numbers.emplace_back(constant(bcirc, i, ceil_log2(nvals)));
    }
    const_idx = vcombine_bool(numbers);
    assert(const_idx.get_nvals() == nvals);

#ifndef DEBUG_SEL_RESULT
    CircUnit W{0};
#endif
    for (auto& w : cfg.hw_weights_r) W += w;
    for (auto& w : cfg.bin_weights_r) W += w;

    CircUnit T = cfg.threshold * (1 << cfg.dice_prec);
    CircUnit Tt = cfg.tthreshold * (1 << cfg.dice_prec);
#ifdef DEBUG_SEL_INPUT
    cout << "W: " << hex << W << " T: " << hex << T << " Tt: " << hex << Tt << endl;
#endif

    const_w_threshold = constant(bcirc, W*T, BitLen);
    const_w_tthreshold = constant(bcirc, W*Tt, BitLen);
#ifdef DEBUG_SEL_CIRCUIT
    print_share(const_zero, "const_zero");
    print_share(const_idx, "const_idx");
    print_share(const_w_threshold , "const_w_threshold ");
    print_share(const_w_tthreshold , "const_w_tthreshold ");
#endif
  }

  BoolShare best_group_weight_hw(const IndexSet& group_set) {
    vector<size_t> group{begin(group_set), end(group_set)};
    // copy group to store permutations
    vector<size_t> groupPerm{group.begin(), group.end()};
    size_t size = group.size();
    vector<BoolShare> perm_weights; // where we store all weights before max
    perm_weights.reserve(factorial<size_t>(size));
    // iterate over all group permutations and calc weight
    do {
      vector<ArithShare> a_field_weights;
      a_field_weights.reserve(size);
      for (size_t i = 0; i != size; ++i) {
        size_t ileft = group[i];
        size_t iright = groupPerm[i];
        a_field_weights.emplace_back(weight_compare_hw(ileft, iright));
      }
      // sum all field-weights for this permutation
      ArithShare sum_perm_weight{sum(a_field_weights)};
#ifdef DEBUG_SEL_CIRCUIT
      print_share(sum_perm_weight, "sum_perm_weight");
#endif
      // convert back to bool and collect for later max
      perm_weights.emplace_back(to_bool(sum_perm_weight));
    } while(next_permutation(groupPerm.begin(), groupPerm.end()));
    // return max of all perm_weights
    BoolShare max_perm_weight{max(perm_weights)};
#ifdef DEBUG_SEL_CIRCUIT
    print_share(max_perm_weight, "max_perm_weight");
#endif
    return max_perm_weight;
  }

  FieldWeight field_weight_hw(size_t ileft, size_t iright) {
    // 1. Calculate weight * delta(i,j)
    // If indices match, use precomputed rescaled weights. Otherwise take
    // arithmetic average of both weights
    CircUnit weight_r = (ileft == iright) ? cfg.hw_weights_r[ileft] :
      rescale_weight((cfg.hw_weights[ileft] + cfg.hw_weights[iright])/2,
          cfg.weight_prec, cfg.max_weight);

    ArithShare a_weight{constant_simd(acirc, weight_r, BitLen, nvals)};
    ArithShare delta = hw_client_delta[ileft] * hw_server_delta[iright];
    ArithShare weight = a_weight * delta; // free constant multiplication

    // 2. Compare values (output dice precision) and multiply with weight
    BoolShare b_comp = compare_hw(hw_client[ileft], hw_server[iright],
        hw_client_hw[ileft], hw_server_hw[iright], cfg.dice_prec);
    ArithShare comp = to_arith(b_comp);
    ArithShare field_weight = weight * comp;

    return {field_weight, weight};
  }

  FieldWeight field_weight_bin(size_t ileft, size_t iright) {
    // 1. Calculate weight * delta(i,j)
    // If indices match, use precomputed rescaled weights. Otherwise take
    // arithmetic average of both weights
    CircUnit weight_r = (ileft == iright) ? cfg.bin_weights_r[ileft] :
      rescale_weight((cfg.bin_weights[ileft] + cfg.bin_weights[iright])/2,
          cfg.weight_prec, cfg.max_weight);

    ArithShare a_weight{constant_simd(acirc, weight_r, BitLen, nvals)};
    ArithShare delta = bin_client_delta[ileft] * bin_server_delta[iright];
    ArithShare weight = a_weight * delta; // free constant multiplication

    // 2. Compare values (shift by dice precision) and multiply with weight
    BoolShare b_comp = compare_bin(bin_client[ileft], bin_server[iright]);
    b_comp = b_comp << cfg.dice_prec;
    ArithShare comp = to_arith(b_comp);
    ArithShare field_weight = weight * comp;

    return {field_weight, weight};
  }

  /*
   * Calculate a single hw summand in the weight sum:
   *  - compare_hw (2 * HW(x & y) / (HW(x) + HW(y))
   *  - mux to zero if any field is empty
   *  - convert compare_hw to arith
   *  - arith-const of weight_r (simd nvals)
   *  - multiply and return
   *
   * @param ileft index of x
   * @param iright index of y
   */
  ArithShare weight_compare_hw(size_t ileft, size_t iright) {
    BoolShare comp = compare_hw(hw_client[ileft], hw_server[iright],
        hw_client_hw[ileft], hw_server_hw[iright], cfg.dice_prec);

    // TODO or first convert to Arith, then multiply with 1/0
    comp = (hw_client_empty[ileft] | hw_server_empty[iright]).mux(const_zero, comp);
#ifdef DEBUG_SEL_CIRCUIT
    print_share(comp, fmt::format("^^^^ HW Comparison of pair ({},{}) ^^^^", ileft, iright));
#endif
    // now go arithmetic
    ArithShare a_comp{to_arith(comp)};
    // If indices match, use precomputed rescaled weights. Otherwise take
    // arithmetic average of both weights
    CircUnit weight_r = (ileft == iright) ? cfg.hw_weights_r[ileft] :
      rescale_weight((cfg.hw_weights[ileft] + cfg.hw_weights[iright])/2,
          cfg.weight_prec, cfg.max_weight);

    ArithShare a_weight{constant_simd(acirc, weight_r, BitLen, nvals)};

    ArithShare a_field_weight{a_comp * a_weight};
#ifdef DEBUG_SEL_CIRCUIT
    print_share(a_comp, format("a_comp({},{})", ileft, iright));
    print_share(a_weight, format("a_weight({},{})", ileft, iright));
    print_share(a_field_weight, "a_field_weight");
#endif
    return a_field_weight;
  }

  /*
   * Calculate a single bin summand in the weight sum:
   *  - compare_bin (x == y)
   *  - convert compare_bin to arith
   *  - arith-const of weight_r (simd nvals)
   *  - multiply and return
   *
   * @param ileft index of x
   * @param iright index of y
   */
  BoolShare weight_compare_bin(size_t ileft, size_t iright) {
    BoolShare comp = compare_bin(bin_client[ileft], bin_server[iright]);

    comp = (~bin_client_empty[ileft] & ~bin_server_empty[iright] & comp);
#ifdef DEBUG_SEL_CIRCUIT
    print_share(comp, fmt::format("^^^^ BIN Comparison of pair ({},{}) ^^^^", ileft, iright));
#endif
    // If indices match, use precomputed rescaled weights. Otherwise take
    // arithmetic average of both weights
    CircUnit weight_r = (ileft == iright) ? cfg.bin_weights_r[ileft] :
      rescale_weight((cfg.bin_weights[ileft] + cfg.bin_weights[iright])/2,
          cfg.weight_prec, cfg.max_weight);

    weight_r <<= cfg.dice_prec; // same shift as in HW field-weights

    BoolShare b_weight{constant_simd(bcirc, weight_r,
        cfg.dice_prec + cfg.weight_prec, nvals)};
    BoolShare b_field_weight{comp.mux(b_weight, const_zero)};

#ifdef DEBUG_SEL_CIRCUIT
    print_share(b_weight, format("b_weight({},{})", ileft, iright));
    print_share(b_field_weight, "b_field_weight");
#endif
    return b_field_weight;
  }

};

/******************** Public Epilinker Interface ********************/
SecureEpilinker::SecureEpilinker(ABYConfig config, EpilinkConfig epi_config) :
  party{config.role, config.remote_host.c_str(), config.port, LT, BitLen, config.nthreads},
  bcirc{dynamic_cast<BooleanCircuit*>(party.GetSharings()[config.bool_sharing]->GetCircuitBuildRoutine())},
  ccirc{dynamic_cast<BooleanCircuit*>(party.GetSharings()[(config.bool_sharing==S_YAO)?S_BOOL:S_YAO]->GetCircuitBuildRoutine())},
  acirc{dynamic_cast<ArithmeticCircuit*>(party.GetSharings()[S_ARITH]->GetCircuitBuildRoutine())},
  epicfg{epi_config}, selc{make_unique<SELCircuit>(epicfg, bcirc, ccirc, acirc)} {}
// TODO when ABY can separate circuit building/setup/online phases, we create
// different SELCircuits per build_circuit()...

// Need to _declare_ in header but _define_ here because we use a unique_ptr
// pimpl.
SecureEpilinker::~SecureEpilinker() = default;

void SecureEpilinker::build_circuit(const uint32_t) {
  // TODO When separation of setup, online phase and input setting is done in
  // ABY, call selc->build_circuit() here instead of in run()
  // nvals is currently ignored as nvals is just taken from EpilinkInputs
  is_built = true;
}

void SecureEpilinker::run_setup_phase() {
  if (!is_built) {
    throw runtime_error("Circuit must first be built with build_circuit() before running setup phase.");
  }
  is_setup = true;
}

SecureEpilinker::Result SecureEpilinker::run_as_client(
    const EpilinkClientInput& input) {
  if (!is_setup) {
    cerr << "Warning: Implicitly running setup phase." << endl;
    run_setup_phase();
  }
  selc->set_client_input(input);
  return run_circuit();
}

SecureEpilinker::Result SecureEpilinker::run_as_server(
    const EpilinkServerInput& input) {
  if (!is_setup) {
    cerr << "Warning: Implicitly running setup phase." << endl;
    run_setup_phase();
  }
  selc->set_server_input(input);
  return run_circuit();
}

#ifdef DEBUG_SEL_CIRCUIT
SecureEpilinker::Result SecureEpilinker::run_as_both(
    const EpilinkClientInput& in_client, const EpilinkServerInput& in_server) {
  if (!is_setup) {
    cerr << "Warning: Implicitly running setup phase." << endl;
    run_setup_phase();
  }
  selc->set_both_inputs(in_client, in_server);
  return run_circuit();
}
#endif

SecureEpilinker::Result SecureEpilinker::run_circuit() {
  SELCircuit::ResultShares res = selc->build_circuit();
  party.ExecCircuit();

  is_setup = false; // need to run new setup phase

  return {
    res.index.get_clear_value<CircUnit>(),
    res.match.get_clear_value<bool>(),
    res.tmatch.get_clear_value<bool>()
#ifdef DEBUG_SEL_RESULT
    ,res.score_numerator.get_clear_value<CircUnit>()
    ,res.score_denominator.get_clear_value<CircUnit>()
#endif
  };
}

void SecureEpilinker::reset() {
  party.Reset();
  is_built = false;
  is_setup = false;
}

/*
  * Takes rescaled weights and makes Contant Input Shares on the given circuit
  * run VCircUnit weights_rsc = rescale_weights(weights); before
  */
vector<Share> make_weight_inputs(Circuit* circ, const VCircUnit& weights_rsc,
    uint32_t bitlen, uint32_t nvals) {
  vector<Share> inputs(weights_rsc.size());
  transform(weights_rsc.cbegin(), weights_rsc.cend(), inputs.begin(),
      [&circ, &bitlen, &nvals](CircUnit w) {
        return constant_simd(circ, w, bitlen, nvals);
      });
  return inputs;
}

/*
  * Create BoolShare inputs of client bitmasks
  */
vector<BoolShare> make_client_bitmask_inputs(BooleanCircuit* circ,
    const VBitmask& bitmasks, uint32_t bitlen, uint32_t nvals) {
  vector<BoolShare> inputs(bitmasks.size());
  transform(bitmasks.cbegin(), bitmasks.cend(), inputs.begin(),
      [&circ, &bitlen, &nvals](Bitmask b) {
        return BoolShare{circ, b.data(), bitlen, CLIENT}.repeat(nvals);
      });
  return inputs;
}

} // namespace sel
