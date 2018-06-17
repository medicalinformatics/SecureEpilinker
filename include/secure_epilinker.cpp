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
#ifdef DEBUG_SEL_CIRCUIT
#include "fmt/ostream.h"
using fmt::format, fmt::print;
#endif
#include "util.h"
#include "secure_epilinker.h"
#include "epilink_input.h"
#include "math.h"
#include "aby/Share.h"
#include "aby/gadgets.h"
#include "seltypes.h"

using namespace std;

namespace sel {

constexpr auto BIN = FieldComparator::BINARY;
constexpr auto BM = FieldComparator::NGRAM;
constexpr auto FieldTypes = {BIN, BM}; // supported field types

/***************** Circuit gadgets *******************/
/**
  * Return type of weight_compare_*()
  * fw - field weight = weight * comparison * empty-flags
  * w - weight for weight sum = weight * empyt-flags
  */
struct FieldWeight { ArithShare fw, w; };

#ifdef DEBUG_SEL_CIRCUIT
void print_share(const FieldWeight& q, const string& msg) {
  print_share(q.fw, msg + "(field-w)");
  print_share(q.w, msg + "(weight)");
}
#endif

/**
 * Sums all fw's and w's in given vector and returns sums as ArithQuotient
 */
ArithQuotient sum(const vector<FieldWeight>& fweights) {
  size_t size = fweights.size();
  vector<ArithShare> fws, ws;
  fws.reserve(size);
  ws.reserve(size);
  for (const auto& fweight : fweights) {
    fws.emplace_back(fweight.fw);
    ws.emplace_back(fweight.w);
  }
  return {sum(fws), sum(ws)};
}

/**
 * Calculates dice coefficient of given bitmasks and their hamming weights, up
 * to the specified precision
 */
BoolShare dice_coefficient(const BoolShare& x, const BoolShare& y,
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
  BoolShare dice = apply_file_binary(hw_and_shifted, hw_plus, 16, 16, "circ/int_div_16.aby");
#ifdef DEBUG_SEL_CIRCUIT
  print_share(dice, "dice coeff");
#endif
  return dice;
}

/*
 * Binary-compares two shares
 */
BoolShare equality(const BoolShare& x, const BoolShare& y) {
  BoolShare cmp{(x == y)};
#ifdef DEBUG_SEL_CIRCUIT
  print_share(cmp, "equality");
#endif
  return cmp;
}

/******************** Circuit Builder ********************/
class SecureEpilinker::SELCircuit {
public:
  SELCircuit(EpilinkConfig cfg,
    BooleanCircuit* bcirc, BooleanCircuit* ccirc, ArithmeticCircuit* acirc) :
    cfg{cfg}, bcirc{bcirc}, ccirc{ccirc}, acirc{acirc},
    ins{
      {BM, vector<InputShares>{cfg.nfields.at(BM)}},
      {BIN, vector<InputShares>{cfg.nfields.at(BIN)}}
    },
    to_arith_closure{[this](auto x){return to_arith(x);}},
    to_bool_closure{[this](auto x){return to_bool(x);}}
  {}

  void set_client_input(const EpilinkClientInput& input) {
    set_constants(input.nvals);
    set_real_client_input(input);
    set_dummy_server_input();

    is_input_set = true;
  }

  void set_server_input(const EpilinkServerInput& input) {
    set_constants(input.nvals);
    set_real_server_input(input);
    set_dummy_client_input();

    is_input_set = true;
  }

#ifdef DEBUG_SEL_CIRCUIT
  /**
   * Debugging test_both_inputs() to test exactly mirrored inputs.
   */
  void set_both_inputs(const EpilinkClientInput& in_client, const EpilinkServerInput& in_server) {
    set_constants(in_client.nvals);
    set_real_client_input(in_client);
    set_real_server_input(in_server);

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
    vector<FieldWeight> field_weights;
    //field_weights.reserve(cfg.bm_exchange_groups.size() + cfg.bin_exchange_groups.size());

    // 1. Field weights of individual fields
    // 1.1 For all exchange groups, find the permutation with the highest score
    for (const FieldComparator ftype : FieldTypes) {
      // Where we collect indices not already used in an exchange group
      set<size_t> no_x_group;
      // fill with ascending indices, remove used ones later
      for (size_t i = 0; i != cfg.nfields.at(ftype); ++i) no_x_group.emplace(i);
      // for each group, store the best permutation's weight into field_weights
      for (const auto& group : cfg.exchange_groups.at(ftype)) {
        // add this group's field weight to vector
        field_weights.emplace_back(best_group_weight(group, ftype));
        // remove all indices that were covered by this index group
        for (const size_t& i : group) {
          size_t r = no_x_group.erase(i);
          // TODO better check this already in EpilinkConfig constructor
          if (!r) throw runtime_error("Exchange groups must be distinct!");
        }
      }
      // 1.2 Remaining indices
      for (size_t i : no_x_group) {
        field_weights.emplace_back(field_weight(i, i, ftype));
      }
    }

    // 2. Sum up all field weights.
    ArithQuotient sum_field_weights{sum(field_weights)};
#ifdef DEBUG_SEL_CIRCUIT
    print_share(sum_field_weights, "sum_field_weights");
#endif

    // 4. Determine index of max score of all nvals calculations
    // Create targets vector with const_idx copy to pass to
    // split_select_quotient_target().
    vector<BoolShare> max_idx{1, const_idx};
    assert((bool)const_idx); // check that it was copied, not moved
    split_select_quotient_target(sum_field_weights, max_idx,
        make_max_selector(to_bool_closure), to_arith_closure);

    // 5. Left side: sum up all weights and multiply with threshold
    //  Since weights and threshold are public inputs, this can be computed
    //  locally. This is done in set_constants().
    //
    // 6. Set two comparison bits, whether > (tentative) threshold
    BoolShare threshold_weight = to_bool(const_threshold * sum_field_weights.den);
    BoolShare tthreshold_weight = to_bool(const_tthreshold * sum_field_weights.den);
    BoolShare b_sum_field_weight = to_bool(sum_field_weights.num);
    BoolShare match = threshold_weight < b_sum_field_weight;
    BoolShare tmatch = tthreshold_weight < b_sum_field_weight;
#ifdef DEBUG_SEL_CIRCUIT
    print_share(sum_field_weights, "best score");
    print_share(max_idx[0], "index of best score");
    print_share(threshold_weight, "T*W");
    print_share(tthreshold_weight, "Tt*W");
    print_share(match, "match?");
    print_share(tmatch, "tentative match?");
#endif

#ifdef DEBUG_SEL_RESULT
      //TODO Actually return W = sum of non-empty field weights
    return {out(max_idx[0], ALL), out(match, ALL), out(tmatch, ALL),
      out(sum_field_weights.num, ALL),
      out(sum_field_weights.den, ALL)};
#else
    return {out_shared(max_idx[0]), out_shared(match), out_shared(tmatch)};
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
    BoolShare empty; // empty flag - will probably be kicked out
    ArithShare delta; // 1 if non-empty, 0 o/w
    BoolShare hw; // precomputed HW of val - not used for bin
  };
  struct InputShares {
    ValueShare client, server;
  };
  /**
   * All input shares are stored in this map, keyed by field/comparator type
   * (BIN = equality test; BM = Hammingweight dice coefficient)
   * Value is a vector over all fields, and each field holds the ValueShare for
   * client and server. A ValueShare holds the value of the field itself, a
   * delta flag, which is 1 if the field is non-empty, and the precalculated
   * hammingweight for bitmasks.
   */
  map<FieldComparator, vector<InputShares>> ins;
  // Constant shares
  BoolShare const_zero, const_idx;
  // Left side of inequality: T * sum(weights)
  ArithShare const_threshold, const_tthreshold;

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

  // closures
  const A2BConverter to_bool_closure;
  const B2AConverter to_arith_closure;

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
    for (auto& w : cfg.weights.at(BM))
      W += rescale_weight(w, cfg.weight_prec, cfg.max_weight);
    for (auto& w : cfg.weights.at(BIN))
      W += rescale_weight(w, cfg.weight_prec, cfg.max_weight);

    CircUnit T = cfg.threshold * (1 << cfg.dice_prec);
    CircUnit Tt = cfg.tthreshold * (1 << cfg.dice_prec);
#ifdef DEBUG_SEL_INPUT
    cout << "W: " << hex << W << " T: " << hex << T << " Tt: " << hex << Tt << endl;
#endif

    const_threshold = constant(acirc, T, BitLen);
    const_tthreshold = constant(acirc, Tt, BitLen);
#ifdef DEBUG_SEL_CIRCUIT
    print_share(const_zero, "const_zero");
    print_share(const_idx, "const_idx");
    print_share(const_threshold , "const_threshold ");
    print_share(const_tthreshold , "const_tthreshold ");
#endif
  }

  void set_real_client_input(const EpilinkClientInput& input) {
    assert (nvals > 0 && "call set_constants() before set_*_input()");

    for (size_t i = 0; i != cfg.nfields.at(BM); ++i) {
      // bitmask records are saved as vector<uint8_t>, so need to access raw data
      check_vector_size(input.bm_record[i], cfg.bytes_bitmask, "rec bitmask byte vector");
      ins[BM][i].client.val = BoolShare(bcirc,
          repeat_vec(input.bm_record[i], nvals).data(),
          cfg.size_bitmask, CLIENT, nvals);

      ins[BM][i].client.hw = BoolShare(bcirc,
          vector<CircUnit>(nvals, hw(input.bm_record[i])).data(),
          cfg.size_hw, CLIENT, nvals);

      // need to convert bool into uint8_t array
      ins[BM][i].client.empty = BoolShare(bcirc,
          vector<uint8_t>(nvals, input.bm_rec_empty[i]).data(),
          1, CLIENT, nvals);

      ins[BM][i].client.delta = ArithShare(acirc,
          vector<CircUnit>(nvals, !input.bm_rec_empty[i]).data(),
          BitLen, CLIENT, nvals);
#ifdef DEBUG_SEL_CIRCUIT
      print_share(ins[BM][i].client.val, format("bm.client.val[{}]", i));
      print_share(ins[BM][i].client.hw, format("bm.client.hw[{}]", i));
      print_share(ins[BM][i].client.empty, format("bm.client.empty[{}]", i));
#endif
    }

    for (size_t i = 0; i != cfg.nfields.at(BIN); ++i) {
      ins[BIN][i].client.val = BoolShare(bcirc,
          vector<CircUnit>(nvals, input.bin_record[i]).data(),
          BitLen, CLIENT, nvals);

      ins[BIN][i].client.empty = BoolShare(bcirc,
          vector<uint8_t>(nvals, input.bin_rec_empty[i]).data(),
          1, CLIENT, nvals);

      ins[BIN][i].client.delta = ArithShare(acirc,
          vector<CircUnit>(nvals, !input.bin_rec_empty[i]).data(),
          BitLen, CLIENT, nvals);
#ifdef DEBUG_SEL_CIRCUIT
      print_share(ins[BIN][i].client.val, format("bin.client.val[{}]", i));
      print_share(ins[BIN][i].client.empty, format("bin.client.empty[{}]", i));
#endif
    }
  }

  void set_real_server_input(const EpilinkServerInput& input) {
    assert (nvals > 0 && "call set_constants() before set_*_input()");

    vector<CircUnit> bm_db_delta(nvals);
    for (size_t i = 0; i != cfg.nfields.at(BM); ++i) {
      check_vectors_size(input.bm_database[i], cfg.bytes_bitmask, " db bitmask byte vectors");
      ins[BM][i].server.val = BoolShare(bcirc,
          concat_vec(input.bm_database[i]).data(),
          cfg.size_bitmask, SERVER, nvals);

      ins[BM][i].server.hw = BoolShare(bcirc,
          hw(input.bm_database[i]).data(),
          cfg.size_hw, SERVER, nvals);

      // need to convert bool array into uint8_t array
      ins[BM][i].server.empty = BoolShare(bcirc,
         vector<uint8_t>(input.bm_db_empty[i].cbegin(),
           input.bm_db_empty[i].cend()).data(),
         1, SERVER, nvals);

      for (size_t j=0; j!=nvals; ++j) bm_db_delta[j] = !input.bm_db_empty[i][j];
      ins[BM][i].server.delta = ArithShare(acirc, bm_db_delta.data(),
          BitLen, SERVER, nvals);
#ifdef DEBUG_SEL_CIRCUIT
      print_share(ins[BM][i].server.val, format("bm.server.val[{}]", i));
      print_share(ins[BM][i].server.hw, format("bm.server.hw[{}]", i));
      print_share(ins[BM][i].server.empty, format("bm.server.empty[{}]", i));
#endif
    }

    vector<CircUnit> bin_db_delta(nvals);
    for (size_t i = 0; i != cfg.nfields.at(BIN); ++i) {
      ins[BIN][i].server.val = BoolShare(bcirc,
          const_cast<CircUnit*>(input.bin_database[i].data()),
          BitLen, SERVER, nvals);

      ins[BIN][i].server.empty = BoolShare(bcirc,
         vector<uint8_t>(input.bin_db_empty[i].cbegin(),
           input.bin_db_empty[i].cend()).data(),
          1, SERVER, nvals);

      for (size_t j=0; j!=nvals; ++j) bin_db_delta[j] = !input.bin_db_empty[i][j];
      ins[BIN][i].server.delta = ArithShare(acirc, bin_db_delta.data(),
          BitLen, SERVER, nvals);
#ifdef DEBUG_SEL_CIRCUIT
      print_share(ins[BIN][i].server.val, format("bin.server.val[{}]", i));
      print_share(ins[BIN][i].server.empty, format("bin.server.empty[{}]", i));
#endif
    }
  }

  void set_dummy_client_input() {
    assert (nvals > 0 && "call set_constants() before set_*_input()");

    for (size_t i = 0; i != cfg.nfields.at(BM); ++i) {
      ins[BM][i].client.val = BoolShare(bcirc, cfg.size_bitmask, nvals); //dummy

      ins[BM][i].client.hw = BoolShare(bcirc, cfg.size_hw, nvals); //dummy

      ins[BM][i].client.empty = BoolShare(bcirc, 1, nvals); // dummy

      ins[BM][i].client.delta = ArithShare(acirc, BitLen, nvals); // dummy
#ifdef DEBUG_SEL_CIRCUIT
      print_share(ins[BM][i].client.val, format("bm.client.val[{}]", i));
      print_share(ins[BM][i].client.hw, format("bm.client.hw[{}]", i));
      print_share(ins[BM][i].client.empty, format("bm.client.empty[{}]", i));
#endif
    }

    for (size_t i = 0; i != cfg.nfields.at(BIN); ++i) {
      ins[BIN][i].client.val = BoolShare(bcirc, BitLen, nvals); //dummy

      ins[BIN][i].client.empty = BoolShare(bcirc, 1, nvals); // dummy

      ins[BIN][i].client.delta = ArithShare(acirc, BitLen, nvals); // dummy
#ifdef DEBUG_SEL_CIRCUIT
      print_share(ins[BIN][i].client.val, format("bin.client.val[{}]", i));
      print_share(ins[BIN][i].client.empty, format("bin.client.empty[{}]", i));
#endif
    }
  }

  void set_dummy_server_input() {
    assert (nvals > 0 && "call set_constants() before set_*_input()");

    for (size_t i = 0; i != cfg.nfields.at(BM); ++i) {
      ins[BM][i].server.val = BoolShare(bcirc, cfg.size_bitmask, nvals); //dummy

      ins[BM][i].server.hw = BoolShare(bcirc, cfg.size_hw, nvals); //dummy

      ins[BM][i].server.empty = BoolShare(bcirc, 1, nvals); // dummy

      ins[BM][i].server.delta = ArithShare(acirc, BitLen, nvals); // dummy
#ifdef DEBUG_SEL_CIRCUIT
      print_share(ins[BM][i].server.val, format("bm.server.val[{}]", i));
      print_share(ins[BM][i].server.hw, format("bm.server.hw[{}]", i));
      print_share(ins[BM][i].server.empty, format("bm.server.empty[{}]", i));
#endif
    }

    for (size_t i = 0; i != cfg.nfields.at(BIN); ++i) {
      ins[BIN][i].server.val = BoolShare(bcirc, BitLen, nvals); //dummy

      ins[BIN][i].server.empty = BoolShare(bcirc, 1, nvals); // dummy

      ins[BIN][i].server.delta = ArithShare(acirc, BitLen, nvals); // dummy
#ifdef DEBUG_SEL_CIRCUIT
      print_share(ins[BIN][i].server.val, format("bin.server.val[{}]", i));
      print_share(ins[BIN][i].server.empty, format("bin.server.empty[{}]", i));
#endif
    }
  }

  FieldWeight best_group_weight(const IndexSet& group_set,
      const FieldComparator& ftype) {
    vector<size_t> group{begin(group_set), end(group_set)};
    // copy group to store permutations
    vector<size_t> groupPerm{group.begin(), group.end()};
    size_t size = group.size();
    vector<ArithQuotient> perm_weights; // where we store all weights before max
    perm_weights.reserve(factorial<size_t>(size));
    // iterate over all group permutations and calc field-weight
    do {
      vector<FieldWeight> field_weights;
      field_weights.reserve(size);
      for (size_t i = 0; i != size; ++i) {
        size_t ileft = group[i];
        size_t iright = groupPerm[i];
        field_weights.emplace_back(field_weight(ileft, iright, ftype));
      }
      // sum all field-weights for this permutation
      ArithQuotient sum_perm_weight{sum(field_weights)};
#ifdef DEBUG_SEL_CIRCUIT
      print_share(sum_perm_weight,
          //format("sum_perm_weight ({},{})", ftype, groupPerm)); // TODO#19
          format("sum_perm_weight ({})", ftype ));
#endif
      // collect for later max
      perm_weights.emplace_back(sum_perm_weight);
    } while(next_permutation(groupPerm.begin(), groupPerm.end()));
    // return max of all perm_weights
    ArithQuotient max_perm_weight{max(perm_weights, to_bool_closure, to_arith_closure)};
#ifdef DEBUG_SEL_CIRCUIT
    print_share(max_perm_weight,
        //format("max_perm_weight ({},{})", ftype, group)); // TODO#19
        format("max_perm_weight ({})", ftype));
#endif
    // Treat quotient as FieldWeight
    return {max_perm_weight.num, max_perm_weight.den};
  }

  /**
   * Calculates the field weight and addend to the total weight.
   * - Set rescaled weight as arithmetic constant
   * - Set weight to 0 if any field on either side is empty
   * - Run comparison, dependent on field type:
   *   - Bitmasks: Dice coefficient with precision dice_prec
   *   - Binary: Simple Equality, but 0/1 shifted to the left by same dice_prec
   * - Multiply result of comparison with weight -> field weight
   * - Return field weight and weight
   */
  FieldWeight field_weight(size_t ileft, size_t iright, FieldComparator ftype) {
    // 1. Calculate weight * delta(i,j)
    // If indices match, use precomputed rescaled weights. Otherwise take
    // arithmetic average of both weights
    CircUnit weight_r = rescale_weight(
        (cfg.weights.at(ftype)[ileft] + cfg.weights.at(ftype)[iright])/2,
          cfg.weight_prec, cfg.max_weight);

    ArithShare a_weight{constant_simd(acirc, weight_r, BitLen, nvals)};

    auto client_field = ins[ftype][ileft].client;
    auto server_field = ins[ftype][iright].server;
    ArithShare delta = client_field.delta * server_field.delta;
    ArithShare weight = a_weight * delta; // free constant multiplication

    // 2. Compare values (with dice precision) and multiply with weight
    BoolShare b_comp;
    ArithShare comp;
    switch(ftype) {
      case BM: {
        b_comp = dice_coefficient(client_field.val, server_field.val,
          client_field.hw, server_field.hw, cfg.dice_prec);
        comp = to_arith(b_comp);
        break;
      }
      case BIN: {
        b_comp = equality(client_field.val, server_field.val);
        // Instead of left-shifting the bool share, it is cheaper to first do a
        // single-bit conversion into an arithmetic share and then a free
        // multiplication with a constant 2^dice_prec
        comp = to_arith(b_comp);
        comp *= constant_simd(acirc, (1 << cfg.dice_prec), BitLen, nvals);
        break;
      }
    }

    ArithShare field_weight = weight * comp;

#ifdef DEBUG_SEL_CIRCUIT
    print_share(weight, format("weight({},{},{})", ftype, ileft, iright));
    print_share(b_comp, format("bool comp({},{},{})", ftype, ileft, iright));
    print_share(comp, format("arith comp({},{},{})", ftype, ileft, iright));
    print_share(field_weight, format("^^^^ field weight({},{},{}) ^^^^", ftype, ileft, iright));
#endif

    return {field_weight, weight};
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
