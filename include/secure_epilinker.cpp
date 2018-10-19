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
#include "fmt/format.h"
using fmt::format;
#include "abycore/sharing/sharing.h"
#include "secure_epilinker.h"
#include "circuit_input.h"
#include "util.h"
#include "math.h"
#include "aby/Share.h"
#include "aby/gadgets.h"
#include "seltypes.h"
#include "logger.h"

using namespace std;

namespace sel {

using State = SecureEpilinker::State;
constexpr auto BIN = FieldComparator::BINARY;
constexpr auto BM = FieldComparator::DICE;

/***************** Circuit gadgets *******************/
/**
  * Return type of weight_compare_*()
  * fw - field weight = weight * comparison * empty-deltas
  * w - weight for weight sum = weight * empyt-deltas
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

/******************** Circuit Builder ********************/
struct LinkageShares {
  BoolShare index, match, tmatch;
#ifdef DEBUG_SEL_RESULT
  ArithShare score_numerator, score_denominator;
#endif
};

struct LinkageOutputShares {
  OutShare index, match, tmatch;
#ifdef DEBUG_SEL_RESULT
  OutShare score_numerator, score_denominator;
#endif
};

struct CountOutputShares {
  OutShare matches, tmatches;
};

class SecureEpilinker::SELCircuit {
public:
  SELCircuit(CircuitConfig cfg_,
    BooleanCircuit* bcirc, BooleanCircuit* ccirc, ArithmeticCircuit* acirc) :
    cfg{cfg_}, bcirc{bcirc}, ccirc{ccirc}, acirc{acirc},
    ins{cfg, bcirc, acirc}, // CircuitInput
    to_bool_closure{[this](auto x){return to_bool(x);}},
    to_arith_closure{[this](auto x){return to_arith(x);}}
  {
    get_logger()->trace("SELCircuit created.");
  }

  template<class EpilinkInput>
  void set_input(const EpilinkInput& input) {
    ins.set(input);
  }

#ifdef DEBUG_SEL_CIRCUIT
  void set_both_inputs(const EpilinkClientInput& in_client, const EpilinkServerInput& in_server) {
    ins.set_both(in_client, in_server);
  }
#endif


  vector<LinkageOutputShares> build_linkage_circuit() {
    if (!ins.is_input_set()) {
      throw new runtime_error("Set the input first before building the ciruit!");
    }

    vector<LinkageOutputShares> output_shares;
    output_shares.reserve(ins.nrecords());
    for (size_t index = 0; index != ins.nrecords(); ++index) {
      output_shares.emplace_back(
          to_linkage_output(build_single_linkage_circuit(index)));
    }

    built = true;
    return output_shares;
  }

  CountOutputShares build_count_circuit() {
    if (!ins.is_input_set()) {
      throw new runtime_error("Set the input first before building the ciruit!");
    }

    vector<LinkageShares> linkage_shares;
    linkage_shares.reserve(ins.nrecords());
    for (size_t index = 0; index != ins.nrecords(); ++index) {
      linkage_shares.emplace_back(build_single_linkage_circuit(index));
    }

    built = true;
    return sum_linkage_shares(linkage_shares);
  }

  State get_state() const {
    return {
      ins.nrecords(),
      ins.dbsize(),
      ins.is_input_set(),
      built
    };
  }

  void reset() {
    ins.clear();
    field_weight_cache.clear();
    built = false;
  }

private:
  const CircuitConfig cfg;
  // Circuits
  BooleanCircuit* bcirc; // boolean circuit for boolean parts
  BooleanCircuit* ccirc; // intermediate conversion circuit
  ArithmeticCircuit* acirc;
  // Input shares
  CircuitInput ins;
  // State
  bool built{false};

  // Dynamic converters, dependent on main bool sharing
  BoolShare to_bool(const ArithShare& s) {
    return (bcirc->GetContext() == S_YAO) ? a2y(bcirc, s) : a2b(bcirc, ccirc, s);
  }

  ArithShare to_arith(const BoolShare& s) {
    return (bcirc->GetContext() == S_YAO) ? y2a(acirc, ccirc, s) : b2a(acirc, s);
  }

  BoolShare to_gmw(const BoolShare& s) {
    return (bcirc->GetContext() == S_YAO) ? y2b(ccirc, s) : s;
  }

  // closures
  const A2BConverter to_bool_closure;
  const B2AConverter to_arith_closure;

  /*
  * Builds the record linkage component of the circuit
  */
  LinkageShares build_single_linkage_circuit(size_t index) {
    get_logger()->trace("Building linkage circuit component {}...", index);

    // Where we store all group and individual comparison weights as ariths
    vector<FieldWeight> field_weights;

    // 1. Field weights of individual fields
    // 1.1 For all exchange groups, find the permutation with the highest score
    // Where we collect indices not already used in an exchange group
    IndexSet no_x_group;
    // fill with field names, remove later
    for (const auto& field : cfg.epi.fields) no_x_group.emplace(field.first);
    // for each group, store the best permutation's weight into field_weights
    for (const auto& group : cfg.epi.exchange_groups) {
      // add this group's field weight to vector
      field_weights.emplace_back(best_group_weight(index, group));
      // remove all indices that were covered by this index group
      for (const auto& i : group) no_x_group.erase(i);
    }
    // 1.2 Remaining indices
    for (const auto& i : no_x_group) {
      field_weights.emplace_back(field_weight({index, i, i}));
    }

    // 2. Sum up all field weights.
    ArithQuotient sum_field_weights = sum(field_weights);
#ifdef DEBUG_SEL_CIRCUIT
    print_share(sum_field_weights, format("[{}] sum_field_weights", index));
#endif

    // 3. Determine index of max score of all nvals calculations
    vector<BoolShare> max_idx = {ins.const_idx()};
    // we summed up `nfields` many weights of size `weight_prec`
    const size_t weight_sum_bits = cfg.weight_prec + ceil_log2(cfg.epi.nfields);
    max_tie_index(sum_field_weights, max_idx,
        to_bool_closure, to_arith_closure,
        weight_sum_bits);

    // 4. Left side: sum up all weights and multiply with threshold
    //  Since weights and threshold are public inputs, this can be computed
    //  locally. This is done in set_constants().
    //
    // 5. Set two comparison bits, whether > (tentative) threshold
    BoolShare threshold_weight = to_bool(ins.const_threshold() * sum_field_weights.den);
    BoolShare tthreshold_weight = to_bool(ins.const_tthreshold() * sum_field_weights.den);
    BoolShare b_sum_field_weight = to_bool(sum_field_weights.num);
    BoolShare match = threshold_weight < b_sum_field_weight;
    BoolShare tmatch = tthreshold_weight < b_sum_field_weight;
#ifdef DEBUG_SEL_CIRCUIT
    print_share(sum_field_weights, format("[{}] best score", index));
    print_share(max_idx[0], format("[{}] index of best score", index));
    print_share(threshold_weight, format("[{}] T*W", index));
    print_share(tthreshold_weight, format("[{}] Tt*W", index));
    print_share(match, format("[{}] match?", index));
    print_share(tmatch, format("[{}] tentative match?", index));
#endif

    get_logger()->trace("Linkage circuit component {} built.", index);

#ifdef DEBUG_SEL_RESULT
    return {max_idx[0], match, tmatch, sum_field_weights.num, sum_field_weights.den};
#else
    return {max_idx[0], match, tmatch};
#endif
  }

  LinkageOutputShares to_linkage_output(const LinkageShares& s) {
    // Output shares should be XOR, not Yao shares
    auto index = to_gmw(s.index);
    auto match = to_gmw(s.match);
    auto tmatch = to_gmw(s.tmatch);
#ifdef DEBUG_SEL_RESULT
    // If result debugging is enabled, we let all parties learn all fields plus
    // the individual {field-,}weight-sums.
    // matching mode flag is ignored - it's basically always on.
    return {out(index, ALL), out(match, ALL), out(tmatch, ALL),
      out(s.score_numerator, ALL), out(s.score_denominator, ALL)};
#else // !DEBUG_SEL_RESULT - Normal productive mode
    return {out_shared(index), out_shared(match), out_shared(tmatch)};
#endif // end ifdef DEBUG_SEL_RESULT
  }

  CountOutputShares sum_linkage_shares(vector<LinkageShares> ls) {
    vector<BoolShare> matches, tmatches;
    const auto n = ls.size();
    matches.reserve(n);
    tmatches.reserve(n);
    for (auto& l : ls) {
      matches.emplace_back(l.match);
      tmatches.emplace_back(l.tmatch);
    }

    return {out(to_gmw(sum(matches)), ALL), out(to_gmw(sum(tmatches)), ALL)};
  }

  FieldWeight best_group_weight(size_t index, const IndexSet& group_set) {
    vector<FieldName> group{begin(group_set), end(group_set)};
    // copy group to store permutations
    vector<FieldName> groupPerm = group;
    size_t size = group.size();

    vector<ArithQuotient> perm_weights; // where we store all weights before max
    perm_weights.reserve(factorial<size_t>(size));
    // iterate over all group permutations and calc field-weight
    do {
      vector<FieldWeight> field_weights;
      field_weights.reserve(size);
      for (size_t i = 0; i != size; ++i) {
        const auto& ileft = group[i];
        const auto& iright = groupPerm[i];
        field_weights.emplace_back(field_weight({index, ileft, iright}));
      }
      // sum all field-weights for this permutation
      ArithQuotient sum_perm_weight{sum(field_weights)};
#ifdef DEBUG_SEL_CIRCUIT
      print_share(sum_perm_weight,
          format("[{}] sum_perm_weight ({}|{})", index, group, groupPerm));
#endif
      // collect for later max
      perm_weights.emplace_back(sum_perm_weight);
    } while (next_permutation(groupPerm.begin(), groupPerm.end()));

    // we summed up `size` many weights of size `weight_prec`
    const size_t weight_sum_bits = cfg.weight_prec + ceil_log2(size);
    ArithQuotient max_perm_weight = max_tie(
        perm_weights, to_bool_closure, to_arith_closure, weight_sum_bits);
#ifdef DEBUG_SEL_CIRCUIT
    print_share(max_perm_weight,
        format("[{}] max_perm_weight ({})", index, group));
#endif
    // Treat quotient as FieldWeight
    return {max_perm_weight.num, max_perm_weight.den};
  }

  /**
   * Cache to store calls to field_weight()
   * Can save half the circuit in permutation groups this way.
   */
  map<ComparisonIndex, FieldWeight> field_weight_cache;

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
  const FieldWeight& field_weight(const ComparisonIndex& i) {
    // Probe cache
    const auto& cache_hit = field_weight_cache.find(i);
    if (cache_hit != field_weight_cache.cend()) {
      get_logger()->trace("field_weight cache hit for {}", i);
      return cache_hit->second;
    }

    auto [client_entry, server_entry] = ins.get(i);
    // 1. Calculate weight * delta(i,j)
    auto a_weight = ins.get_const_weight(i);
    ArithShare delta = client_entry.delta * server_entry.delta;
    ArithShare weight = a_weight * delta; // free constant multiplication

    // 2. Compare values (with dice precision) and multiply with weight
    ArithShare comp;
    const auto ftype = cfg.epi.fields.at(i.left).comparator;
    switch(ftype) {
      case BM: {
        const auto b_comp = dice_coefficient(i);
        comp = to_arith(b_comp);
        break;
      }
      case BIN: {
        const auto b_comp = equality(i);
        // Instead of left-shifting the bool share, it is cheaper to first do a
        // single-bit conversion into an arithmetic share and then a free
        // multiplication with a constant 2^dice_prec
        comp = to_arith(b_comp);
        comp *= ins.const_dice_prec_factor();
        break;
      }
    }

    ArithShare field_weight = weight * comp;

#ifdef DEBUG_SEL_CIRCUIT
    print_share(weight, format("weight ({}){}", ftype, i));
    print_share(comp, format("comp ({}){}", ftype, i));
    print_share(field_weight, format("^^^^ field weight ({}){} ^^^^", ftype, i));
#endif

    FieldWeight ret = {field_weight, weight};
    return field_weight_cache[i] = move(ret);
  }

  /**
  * Calculates dice coefficient of given bitmasks via their hamming weights, up
  * to the configured precision.
  * Note that we use rounding integer division, that is (x+(y/2))/y, because x/y
  * always rounds down, which would lead to a bias.
  */
  BoolShare dice_coefficient(const ComparisonIndex& i) {
    auto [client_entry, server_entry] = ins.get(i);

    BoolShare hw_plus = client_entry.hw + server_entry.hw; // denominator
    BoolShare hw_and_twice = hammingweight(server_entry.val & client_entry.val) << 1; // numerator

    // fixed point rounding integer division
    // hw_size(bitsize) + 1 because we multiply numerator with 2 and denominator is sum
    // of two values of original bitsize. Both are hammingweights.
    const auto bitsize = hw_size(cfg.epi.fields.at(i.left).bitsize) + 1;
    const auto int_div_file_path = format((cfg.circ_dir/"sel_int_div/{}_{}.aby").string(),
        bitsize, cfg.dice_prec);
    BoolShare dice = apply_file_binary(hw_and_twice, hw_plus, bitsize, bitsize, int_div_file_path);

#ifdef DEBUG_SEL_CIRCUIT
    print_share(hw_and_twice, format("hw_and_twice {}", i));
    print_share(hw_plus, format("hw_plus {}", i));
    print_share(dice, format("dice {}", i));
#endif

    return dice;
  }

  /*
  * Binary-compares two shares
  */
  BoolShare equality(const ComparisonIndex& i) {
    auto [client_entry, server_entry] = ins.get(i);
    BoolShare cmp = (client_entry.val == server_entry.val);
#ifdef DEBUG_SEL_CIRCUIT
    print_share(cmp, format("equality {}", i));
#endif
    return cmp;
  }

};

/******************** Public Epilinker Interface ********************/
SecureEpilinker::SecureEpilinker(ABYConfig config, CircuitConfig circuit_config) :
  party{config.role, config.host, config.port, LT, BitLen, config.nthreads},
  bcirc{dynamic_cast<BooleanCircuit*>(party.GetSharings()[config.bool_sharing]->GetCircuitBuildRoutine())},
  ccirc{dynamic_cast<BooleanCircuit*>(party.GetSharings()[(config.bool_sharing==S_YAO)?S_BOOL:S_YAO]->GetCircuitBuildRoutine())},
  acirc{dynamic_cast<ArithmeticCircuit*>(party.GetSharings()[S_ARITH]->GetCircuitBuildRoutine())},
  cfg{circuit_config}, selc{make_unique<SELCircuit>(cfg, bcirc, ccirc, acirc)} {
    get_logger()->debug("SecureEpilinker created.");
  }
// TODO when ABY can separate circuit building/setup/online phases, we create
// different SELCircuits per build_circuit()...

// Need to _declare_ in header but _define_ here because we use a unique_ptr
// pimpl.
SecureEpilinker::~SecureEpilinker() = default;

void SecureEpilinker::connect() {
  const auto& logger = get_logger();
  logger->trace("Connecting ABYParty...");
  // Currently, we only let the aby parties connect, which runs the Base OTs.
  party.InitOnline();
  logger->trace("ABYParty connected.");
}

State SecureEpilinker::get_state() {
  return state;
}

void SecureEpilinker::build_linkage_circuit(const size_t num_records, const size_t database_size) {
  build_circuit(num_records, database_size);
  state.matching_mode = false;
}
void SecureEpilinker::build_count_circuit(const size_t num_records, const size_t database_size) {
  build_circuit(num_records, database_size);
  state.matching_mode = true;
}
void SecureEpilinker::build_circuit(const size_t num_records_, const size_t database_size_) {
  // TODO When separation of setup, online phase and input setting is done in
  // ABY, call selc->build_circuit() here instead of in run()
  // nvals is currently ignored as nvals is just taken from EpilinkInputs
  state.num_records = num_records_;
  state.database_size = database_size_;
  state.built = true;
}

void throw_if_not_built(bool built, const string& origin) {
  if (!built) {
    throw runtime_error("Build circuit with build_*_circuit() before "s+origin+'!');
  }
}

template <class EpilinkInput>
void check_state_for_input(const State& state,
    const EpilinkInput& input) {
  throw_if_not_built(state.built, "setting inputs");
  if (state.num_records < input.num_records || state.database_size < input.database_size) {
    throw runtime_error(format("Built circuit too small for input!"
          " nrecords/dbsize is {}/{} but need {}/{}",
          state.num_records, state.database_size, input.num_records, input.database_size));
  }
}

void SecureEpilinker::run_setup_phase() {
  throw_if_not_built(state.built, "running setup phase");
  state.setup = true;
}

void SecureEpilinker::set_client_input(const EpilinkClientInput& input) {
  check_state_for_input(state, input);
  selc->set_input(input);
  state.input_set = true;
}

void SecureEpilinker::set_server_input(const EpilinkServerInput& input) {
  check_state_for_input(state, input);
  selc->set_input(input);
  state.input_set = true;
}

#ifdef DEBUG_SEL_CIRCUIT
void SecureEpilinker::set_both_inputs(
    const EpilinkClientInput& in_client, const EpilinkServerInput& in_server) {
  assert(in_client.num_records == in_server.num_records
      && in_client.database_size == in_server.database_size)
  check_state_for_input(state, in_client);
  selc->set_both_inputs(in_client, in_server);
  state.input_set = true;
}
#endif

Result<CircUnit> to_clear_value(LinkageOutputShares& res, size_t dice_prec) {
#ifdef DEBUG_SEL_RESULT
    const auto sum_field_weights = res.score_numerator.get_clear_value<CircUnit>();
    // shift by dice-precision to account for precision of threshold, i.e.,
    // get denominator and numerator to same scale
    const auto sum_weights = res.score_denominator.get_clear_value<CircUnit>() << dice_prec;
#else
    const CircUnit sum_field_weights = 0;
    const CircUnit sum_weights = 0;
#endif

  return {
    res.index.get_clear_value<CircUnit>(),
    res.match.get_clear_value<bool>(),
    res.tmatch.get_clear_value<bool>(),
    sum_field_weights, sum_weights
  };
}


vector<Result<CircUnit>> SecureEpilinker::run_linkage() {
  if (!state.setup) {
    get_logger()->warn(
        "SecureEpilinker::run_linkage: Implicitly running setup phase.");
    run_setup_phase();
  }

  auto results = selc->build_linkage_circuit();
  get_logger()->trace("Executing ABYParty Circuit...");
  party.ExecCircuit();
  get_logger()->trace("ABYParty Circuit executed.");

  auto clear_results = transform_vec(results, [dice_prec=cfg.dice_prec](auto r){
        return to_clear_value(r, dice_prec);
      });
  state.reset(); // need to setup new circuit
  return clear_results;
}

CountResult<CircUnit> to_clear_value(CountOutputShares& res) {
  return {
    res.matches.get_clear_value<CircUnit>(),
    res.tmatches.get_clear_value<CircUnit>()
  };
}

CountResult<CircUnit> SecureEpilinker::run_count() {
  if (!state.setup) {
    get_logger()->warn(
        "SecureEpilinker::run_count: Implicitly running setup phase.");
    run_setup_phase();

  }
  auto results = selc->build_count_circuit();
  get_logger()->trace("Executing ABYParty Circuit...");
  party.ExecCircuit();
  get_logger()->trace("ABYParty Circuit executed.");

  auto clear_results = to_clear_value(results);
  state.reset(); // need to setup new circuit
  return clear_results;
}

void SecureEpilinker::State::reset() {
  num_records = 0;
  database_size = 0;
  built = false;
  setup = false;
  input_set = false;
}

void SecureEpilinker::reset() {
  selc->reset();
  party.Reset();
  state.reset();
}

} // namespace sel
