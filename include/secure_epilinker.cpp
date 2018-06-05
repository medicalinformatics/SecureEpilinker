
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

constexpr uint32_t BitLen{32};
constexpr size_t QuotPrecisionBits{6};

namespace sel {

/***************** Circuit gadgets *******************/
BoolShare compare_hw(const BoolShare& x, const BoolShare& y,
    const BoolShare& hw_x, const BoolShare& hw_y) {
  // calc HW of AND and bit-shift to multiply with 2 and get QuotPrecisionBits precision
  // for integer-divivision
  BoolShare hw_and = hammingweight(x & y);
#ifdef DEBUG_SEL_CIRCUIT
  print_share(hw_and, "hw_and");
#endif
  BoolShare hw_and_shifted = hw_and << (QuotPrecisionBits + 1);

  // Add single HWs
  BoolShare hw_plus = hw_x + hw_y;
#ifdef DEBUG_SEL_CIRCUIT
  print_share(hw_plus, "hw_plus");
#endif

  // int-divide
  // TODO may need to zero-pad; maybe do that in apply_...
  BoolShare hw_div = apply_file_binary(hw_and_shifted, hw_plus, 16, 16, "int_div_16");
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
class SELCircuit {
public:
  SELCircuit(const EpilinkConfig& cfg,
    BooleanCircuit* bcirc, BooleanCircuit* ccirc, ArithmeticCircuit* acirc) : cfg{cfg},
    bcirc{bcirc}, ccirc{ccirc}, acirc{acirc},
    hw_client{cfg.nhw_fields}, hw_client_hw{cfg.nhw_fields}, hw_client_empty{cfg.nhw_fields},
    bin_client{cfg.nbin_fields}, bin_client_empty{cfg.nbin_fields},
    hw_server{cfg.nhw_fields}, hw_server_hw{cfg.nhw_fields}, hw_server_empty{cfg.nhw_fields},
    bin_server{cfg.nbin_fields}, bin_server_empty{cfg.nbin_fields}
  {}

  void set_client_input(const EpilinkClientInput& input) {
    set_constants(input.nvals);
    // First create the client and input shares, which differ for client and
    // server run. Then pass to the joint routine.
    for (size_t i = 0; i != cfg.nhw_fields; ++i) {
      // bitmask records are saved as vector<uint8_t>, so need to access raw data
      check_vector_size(input.hw_record[i], cfg.bytes_bitmask, "rec bitmask byte vector");
      hw_client[i] = BoolShare{bcirc, const_cast<bitmask_unit*>(input.hw_record[i].data()), cfg.size_bitmask, CLIENT}
        .repeat(input.nvals);

      hw_client_hw[i] = BoolShare{bcirc, hw(input.hw_record[i]), cfg.size_hw, CLIENT}
        .repeat(input.nvals);

      hw_client_empty[i] = BoolShare{bcirc, static_cast<uint8_t>(input.hw_rec_empty[i]), 1, CLIENT}
        .repeat(input.nvals);

      hw_server[i] = BoolShare{bcirc, cfg.size_bitmask}.repeat(input.nvals); //dummy

      hw_server_hw[i] = BoolShare{bcirc, cfg.size_hw}.repeat(input.nvals); //dummy

      hw_server_empty[i] = BoolShare{bcirc, 1}.repeat(input.nvals); // dummy
    }
    for (size_t i = 0; i != cfg.nbin_fields; ++i) {
      bin_client[i] = BoolShare{bcirc, input.bin_record[i], BitLen, CLIENT}
        .repeat(input.nvals);

      bin_client_empty[i] = BoolShare{bcirc, static_cast<uint8_t>(input.bin_rec_empty[i]), 1, CLIENT}
        .repeat(input.nvals);

      bin_server[i] = BoolShare{bcirc, BitLen}.repeat(input.nvals); //dummy

      bin_server_empty[i] = BoolShare{bcirc, 1}.repeat(input.nvals); // dummy
    }
    is_input_set = true;
  }

  void set_server_input(const EpilinkServerInput& input) {
    set_constants(input.nvals);
    // First create the server and input shares, which differ for client and
    // server run. Then pass to the joint routine.
    // concatenated hw input garbage - needs to be in scope until circuit has executed
    vector<bitmask_type> hw_database_cc(cfg.nhw_fields);
    vector<v_hw_type> hw_database_hw(cfg.nhw_fields);
    for (size_t i = 0; i != cfg.nhw_fields; ++i) {
      hw_client[i] = BoolShare{bcirc, cfg.size_bitmask}.repeat(input.nvals); //dummy

      hw_client_hw[i] = BoolShare{bcirc, cfg.size_hw}.repeat(input.nvals); //dummy

      hw_client_empty[i] = BoolShare{bcirc, 1}.repeat(input.nvals); // dummy

      check_vectors_size(input.hw_database[i], cfg.bytes_bitmask, " db bitmask byte vectors");
      hw_database_cc[i] = concat_vec(input.hw_database[i]);
      hw_server[i] = BoolShare(bcirc, hw_database_cc[i].data(), cfg.size_bitmask, SERVER, input.nvals);

      hw_database_hw[i] = hw(input.hw_database[i]);
      hw_server_hw[i] = BoolShare(bcirc, hw_database_hw[i].data(), cfg.size_hw, SERVER, input.nvals);
      hw_server_empty[i] = BoolShare(bcirc,
         vector_bool_to_bitmask(input.hw_db_empty[i]).data(), 1, CLIENT, input.nvals);
    }
    for (size_t i = 0; i != cfg.nbin_fields; ++i) {
      bin_client[i] = BoolShare{bcirc, BitLen}.repeat(input.nvals); //dummy

      bin_client_empty[i] = BoolShare{bcirc, 1}.repeat(input.nvals); // dummy

      bin_server[i] = BoolShare(bcirc,
          const_cast<bin_type*>(input.bin_database[i].data()),
          BitLen, SERVER, input.nvals);

      bin_server_empty[i] = BoolShare(bcirc,
          vector_bool_to_bitmask(input.bin_db_empty[i]).data(), 1, CLIENT, input.nvals);
    }
    is_input_set = true;
  }

  /*
  * Builds the shared component of the circuit after initial input shares of
  * client and server have been created.
  */
  OutShare build_circuit() {
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
    b_field_weights.emplace_back(to_bool(sum(a_field_weights)));
    BoolShare sum_field_weights{sum(b_field_weights)};
#ifdef DEBUG_SEL_CIRCUIT
    print_share(sum_field_weights, "sum_field_weights");
#endif

    // 4. Determine index of max score of all nvals calculations

    // 5. Left side: sum up all weights and multiply with threshold
    //  Since weights and threshold are public inputs, this can be computed
    //  locally. This is done in set_constants().
  }

private:
  // Epilink config
  const EpilinkConfig& cfg;
  // Circuits
  BooleanCircuit* bcirc; // boolean circuit for boolean parts
  BooleanCircuit* ccirc; // intermediate conversion circuit
  ArithmeticCircuit* acirc;
  // Input shares
  vector<BoolShare> hw_client, hw_client_hw, hw_client_empty,
    bin_client, bin_client_empty,
    hw_server, hw_server_hw, hw_server_empty,
    bin_server, bin_server_empty;
  // Constant shares
  BoolShare const_zero, const_idx;
  // Left side of inequality: T * sum(weights)
  ArithShare const_w_threshold, const_w_tthreshold;

  // state variables
  uint32_t nvals{0};
  bool is_input_set{false};

  // Dynamic converters, dependent on main bool sharing
  BoolShare to_bool(const ArithShare& s) {
    return (bcirc->GetContext() == S_YAO) ? a2y(bcirc, s) : a2b(bcirc, ccirc, s);
  }
  ArithShare to_arith(const BoolShare& s) {
    return (bcirc->GetContext() == S_YAO) ? y2a(acirc, ccirc, s) : b2a(acirc, s);
  }

  void set_constants(uint32_t nvals) {
    this->nvals = nvals;
    // create constants with nvals
    const_zero = constant_simd(bcirc, 0, 1, nvals);
    // build constant index vector
    vector<uint32_t> wires;
    wires.reserve(nvals);
    for (bin_type i = 0; i != nvals; ++i) {
      // TODO WRONG?! in bool share only one bit per wire?! so BitLen wires?!
      wires.emplace_back(constant(bcirc, i, BitLen).get()->get_wire_id(0));
    }
    const_idx = BoolShare{bcirc, new boolshare{wires, static_cast<Circuit*>(bcirc)}};

    uint64_t W{0};
    for (auto& w : cfg.hw_weights_r) W += w;
    for (auto& w : cfg.bin_weights_r) W += w;

    uint64_t T = cfg.threshold * (1 << QuotPrecisionBits);
    uint64_t Tt = cfg.tthreshold * (1 << QuotPrecisionBits);
#ifdef DEBUG_SEL_INPUT
    cout << "W: " << hex << W << " T: " << hex << T << " Tt: " << hex << Tt << endl;
#endif

    const_w_threshold = constant_simd(acirc, W*T, BitLen, nvals);
    const_w_tthreshold = constant_simd(acirc, W*Tt, BitLen, nvals);
  }

  /* Not needed, can compute those values offline
  auto threshold_weights() {
    // 4.1 sum rescaled weights
    vector<ArithShare> a_weights = transform_vec( cfg.hw_weights_r,
        function<ArithShare(const hw_type&)>([this](const auto& w)
          {return constant(acirc, w, BitLen);}) );
    ArithShare sum_weights{sum(a_weights)};
#ifdef DEBUG_SEL_CIRCUIT
    for (auto& w : a_weights) print_share(w, "a_weight");
    print_share(sum_weights, "sum_weights");
#endif
    // 4.2 Use thresholds with QuotPrecision
    ArithShare a_threshold = constant(acirc,
        static_cast<uint32_t>(cfg.threshold * (1 << QuotPrecisionBits)), BitLen);
    ArithShare a_tthreshold = constant(acirc,
        static_cast<uint32_t>(cfg.tthreshold * (1 << QuotPrecisionBits)), BitLen);

    ArithShare threshold_weight = sum_weights * a_threshold;
    ArithShare tthreshold_weight = sum_weights * a_tthreshold;
#ifdef DEBUG_SEL_CIRCUIT
    print_share(a_threshold, "a_threshold");
    print_share(a_tthreshold, "a_tthreshold");
    print_share(threshold_weight, "threshold_weight");
    print_share(tthreshold_weight, "tthreshold_weight");
#endif
    threshold_weight = threshold_weight.repeat(nvals);
    tthreshold_weight = tthreshold_weight.repeat(nvals);
    struct ret { ArithShare t; ArithShare tt; };
    return ret {threshold_weight, tthreshold_weight};
  }
  */

  BoolShare best_group_weight_hw(const v_idx& group_set) {
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

  /*
   * Calculate a single hw summand in the weight sum:
   *  - compare_hw (2 * HW(x & y) / (HW(x) + HW(y))
   *  - convert compare_hw to arith
   *  - arith-const of weight_r (simd nvals)
   *  - multiply and return
   *
   * @param ileft index of x
   * @param iright index of y
   */
  ArithShare weight_compare_hw(size_t ileft, size_t iright) {
    BoolShare comp = compare_hw(hw_client[ileft], hw_server[iright],
        hw_client_hw[ileft], hw_server_hw[iright]);

    // TODO or first convert to Arith, then multiply with 1/0
    comp = (hw_client_empty[ileft] | hw_server_empty[iright]).mux(const_zero, comp);
#ifdef DEBUG_SEL_CIRCUIT
    print_share(comp, fmt::format("^^^^ HW Comparison of pair ({},{}) ^^^^", ileft, iright));
#endif
    // now go arithmetic
    ArithShare a_comp{to_arith(comp)};
    // If indices match, use precomputed rescaled weights. Otherwise take
    // arithmetic average of both weights
    hw_type weight_r = (ileft == iright) ? cfg.hw_weights_r[ileft] :
      rescale_weight((cfg.hw_weights[ileft] + cfg.hw_weights[iright])/2, cfg.max_weight);

    ArithShare a_weight{constant_simd(acirc, weight_r, BitLen, nvals)};

    ArithShare a_field_weight{a_comp * a_weight};
#ifdef DEBUG_SEL_CIRCUIT
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
    BoolShare comp = compare_bin(hw_client[ileft], hw_server[iright]);

    comp = (!bin_client_empty[ileft] & !bin_server_empty[iright] & comp);
#ifdef DEBUG_SEL_CIRCUIT
    print_share(comp, fmt::format("^^^^ BIN Comparison of pair ({},{}) ^^^^", ileft, iright));
#endif
    // If indices match, use precomputed rescaled weights. Otherwise take
    // arithmetic average of both weights
    bin_type weight_r = (ileft == iright) ? cfg.bin_weights_r[ileft] :
      rescale_weight((cfg.bin_weights[ileft] + cfg.bin_weights[iright])/2, cfg.max_weight);

    weight_r <<= QuotPrecisionBits; // same shift as in HW field-weights

    BoolShare b_weight{constant_simd(bcirc, weight_r, BitLen, nvals)};
    BoolShare b_field_weight{comp.mux(b_weight, const_zero)};

#ifdef DEBUG_SEL_CIRCUIT
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
  epicfg{epi_config}, selc{make_unique<SELCircuit>(epi_config, bcirc, ccirc, acirc)} {}
// TODO when ABY can separate circuit building/setup/online phases, we create
// different SELCircuits per build_circuit()...

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

uint32_t SecureEpilinker::run_as_client(const EpilinkClientInput& input) {
  if (!is_setup) {
    cerr << "Warning: Implicitly running setup phase." << endl;
    run_setup_phase();
  }
  selc->set_client_input(input);
  return run();
}

uint32_t SecureEpilinker::run_as_server(const EpilinkServerInput& input) {
  if (!is_setup) {
    cerr << "Warning: Implicitly running setup phase." << endl;
    run_setup_phase();
  }
  selc->set_server_input(input);
  return run();
}

uint32_t SecureEpilinker::run() {
  OutShare result = selc->build_circuit();
  party.ExecCircuit();

  is_setup = false; // need to run new setup phase

  return result.get_clear_value<bin_type>();
}

/*
  * Takes rescaled weights and makes Contant Input Shares on the given circuit
  * run v_hw_type weights_rsc = rescale_weights(weights); before
  */
vector<Share> make_weight_inputs(Circuit* circ, const v_hw_type& weights_rsc,
    uint32_t bitlen, uint32_t nvals) {
  vector<Share> inputs(weights_rsc.size());
  transform(weights_rsc.cbegin(), weights_rsc.cend(), inputs.begin(),
      [&circ, &bitlen, &nvals](hw_type w) {
        return constant_simd(circ, w, bitlen, nvals);
      });
  return inputs;
}

/*
  * Create BoolShare inputs of client bitmasks
  */
vector<BoolShare> make_client_bitmask_inputs(BooleanCircuit* circ,
    const v_bitmask_type& bitmasks, uint32_t bitlen, uint32_t nvals) {
  vector<BoolShare> inputs(bitmasks.size());
  transform(bitmasks.cbegin(), bitmasks.cend(), inputs.begin(),
      [&circ, &bitlen, &nvals](bitmask_type b) {
        return BoolShare{circ, b.data(), bitlen, CLIENT}.repeat(nvals);
      });
  return inputs;
}

} // namespace sel
