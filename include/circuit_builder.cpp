/**
  \file    circuit_builder.cpp
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
  \brief SEL Circuit Builder
  */

#include "circuit_builder.h"
#include "fmt/format.h"
using fmt::format;
#include "util.h"
#include "math.h"
#include "seltypes.h"
#include "logger.h"
#include "aby/Share.h"
#include "aby/gadgets.h"

using namespace std;

namespace sel {

/**
  * Return type of weight_compare_*()
  * fw - field weight = weight * comparison * empty-deltas
  * w - weight for weight sum = weight * empyt-deltas
  */
template <class MultShare>
struct FieldWeight { MultShare fw, w; };

template <class MultShare>
struct LinkageShares {
  BoolShare index, match, tmatch;
#ifdef DEBUG_SEL_RESULT
  MultShare score_numerator, score_denominator;
#endif
};

#ifdef DEBUG_SEL_CIRCUIT
template <class MultShare>
void print_share(const FieldWeight<MultShare>& q, const string& msg) {
  print_share(q.fw, msg + "(field-w)");
  print_share(q.w, msg + "(weight)");
}
#endif

/**
 * Sums all fw's and w's in given vector and returns sums as ArithQuotient
 */
template <class MultShare>
Quotient<MultShare> sum(const vector<FieldWeight<MultShare>>& fweights) {
  size_t size = fweights.size();
  vector<MultShare> fws, ws;
  fws.reserve(size);
  ws.reserve(size);
  for (const auto& fweight : fweights) {
    fws.emplace_back(fweight.fw);
    ws.emplace_back(fweight.w);
  }
  return {sum(fws), sum(ws)};
}

/**
 * EpiLink Circuit Builder
 *
 * Calculates \sum_i \delta_i w_i c_i / sum_i \delta_i w_i with exchange groups
 * The type MultShare determines the type of multiplication to use:
 * ArithShare: Do multiplications in arithmetic circuit space, with conversions
 *   from and to boolean space before and after
 * BoolShare: Stay in boolean circuit space for multiplications
 */
template <class MultShare>
class CircuitBuilder : public CircuitBuilderBase {
public:
  CircuitBuilder(CircuitConfig cfg_,
      BooleanCircuit* bcirc, BooleanCircuit* ccirc, ArithmeticCircuit* acirc) :
    cfg{cfg_}, bcirc{bcirc}, ccirc{ccirc}, acirc{acirc},
    ins{cfg, bcirc, acirc}, // CircuitInput
    to_bool_closure{[this](auto x){return to_bool(x);}},
    to_arith_closure{[this](auto x){return to_arith(x);}}
  {
    get_logger()->trace("CircuitBuilder created.");
  }

  void set_input(const EpilinkClientInput& input) override {
    ins.set(input);
  }

  void set_input(const EpilinkServerInput& input) override {
    ins.set(input);
  }

#ifdef DEBUG_SEL_CIRCUIT
  void set_both_inputs(const EpilinkClientInput& in_client, const EpilinkServerInput& in_server) override {
    ins.set_both(in_client, in_server);
  }
#endif


  std::vector<LinkageOutputShares> build_linkage_circuit() override {
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

  CountOutputShares build_count_circuit() override {
    if (!ins.is_input_set()) {
      throw new runtime_error("Set the input first before building the ciruit!");
    }

    vector<LinkageShares<MultShare>> linkage_shares;
    linkage_shares.reserve(ins.nrecords());
    for (size_t index = 0; index != ins.nrecords(); ++index) {
      linkage_shares.emplace_back(build_single_linkage_circuit(index));
    }

    built = true;
    return sum_linkage_shares(linkage_shares);
  }

  void reset() override {
    ins.clear();
    field_weight_cache.clear();
    built = false;
  }

private:
  static constexpr bool do_arith_mult = std::is_same_v<MultShare, ArithShare>;
  using QuotientShare = Quotient<MultShare>;

  const CircuitConfig cfg;
  // Circuits
  BooleanCircuit* bcirc; // boolean circuit for boolean parts
  BooleanCircuit* ccirc; // intermediate conversion circuit
  ArithmeticCircuit* acirc;
  // Input shares
  CircuitInput<MultShare> ins;
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

  BoolShare to_logic_space(const MultShare& s) {
    if constexpr (do_arith_mult)
      return to_bool(s);
    else
      return s;
  }

  MultShare to_mult_space(const BoolShare& s) {
    if constexpr (do_arith_mult)
      return to_arith(s);
    else
      return s;
  }

  // closures
  const A2BConverter to_bool_closure;
  const B2AConverter to_arith_closure;

  /*
  * Builds the record linkage component of the circuit
  */
  LinkageShares<MultShare> build_single_linkage_circuit(size_t index) {
    get_logger()->trace("Building linkage circuit component {}...", index);

    // Where we store all group and individual comparison weights
    vector<FieldWeight<MultShare>> field_weights;

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
    QuotientShare sum_field_weights = sum(field_weights);
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
    BoolShare threshold_weight = to_logic_space(ins.const_threshold() * sum_field_weights.den);
    BoolShare tthreshold_weight = to_logic_space(ins.const_tthreshold() * sum_field_weights.den);
    BoolShare b_sum_field_weight = to_logic_space(sum_field_weights.num);
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

  LinkageOutputShares to_linkage_output(const LinkageShares<MultShare>& s) {
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

  CountOutputShares sum_linkage_shares(std::vector<LinkageShares<MultShare>> ls) {
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

  FieldWeight<MultShare> best_group_weight(size_t index, const IndexSet& group_set) {
    vector<FieldName> group{begin(group_set), end(group_set)};
    // copy group to store permutations
    vector<FieldName> groupPerm = group;
    size_t size = group.size();

    vector<QuotientShare> perm_weights; // where we store all weights before max
    perm_weights.reserve(factorial<size_t>(size));
    // iterate over all group permutations and calc field-weight
    do {
      vector<FieldWeight<MultShare>> field_weights;
      field_weights.reserve(size);
      for (size_t i = 0; i != size; ++i) {
        const auto& ileft = group[i];
        const auto& iright = groupPerm[i];
        field_weights.emplace_back(field_weight({index, ileft, iright}));
      }
      // sum all field-weights for this permutation
      QuotientShare sum_perm_weight = sum(field_weights);
#ifdef DEBUG_SEL_CIRCUIT
      print_share(sum_perm_weight,
                  format("[{}] sum_perm_weight ({}|{})", index, group, groupPerm));
#endif
      // collect for later max
      perm_weights.emplace_back(sum_perm_weight);
    } while (next_permutation(groupPerm.begin(), groupPerm.end()));

    // we summed up `size` many weights of size `weight_prec`
    const size_t weight_sum_bits = cfg.weight_prec + ceil_log2(size);
    QuotientShare max_perm_weight = max_tie(
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
  std::map<ComparisonIndex, FieldWeight<MultShare>> field_weight_cache;

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
  const FieldWeight<MultShare>& field_weight(const ComparisonIndex& i) {
    // Probe cache
    const auto& cache_hit = field_weight_cache.find(i);
    if (cache_hit != field_weight_cache.cend()) {
      get_logger()->trace("field_weight cache hit for {}", i);
      return cache_hit->second;
    }

    auto [client_entry, server_entry] = ins.get(i);
    // 1. Calculate weight * delta(i,j)
    auto a_weight = ins.get_const_weight(i);
    MultShare delta = client_entry.delta * server_entry.delta;
    MultShare weight = a_weight * delta; // free constant multiplication

    // 2. Compare values (with dice precision) and multiply with weight
    MultShare comp;
    const auto ftype = cfg.epi.fields.at(i.left).comparator;
    switch(ftype) {
      case FieldComparator::DICE: {
        const auto b_comp = dice_coefficient(i);
        comp = to_mult_space(b_comp);
        break;
      }
      case FieldComparator::BINARY: {
        const auto b_comp = equality(i);
        // Instead of left-shifting the bool share, it is cheaper to first do a
        // single-bit conversion into an arithmetic share and then a free
        // multiplication with a constant 2^dice_prec
        comp = to_mult_space(b_comp);
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

    FieldWeight<MultShare> ret = {field_weight, weight};
    return field_weight_cache[i] = move(ret);
  }

  /**
  * Calculates dice coefficient of given bitmasks via their hamming weights, up
  * to the configured precision.
  * Note that we use rounding integer division, that is (x+(y/2))/y, because x/y
  * always rounds down, which would lead to a bias.
  */
  MultShare dice_coefficient(const ComparisonIndex& i) {
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

  /**
  * Binary-compares two shares
  */
  MultShare equality(const ComparisonIndex& i) {
    auto [client_entry, server_entry] = ins.get(i);
        BoolShare cmp = (client_entry.val == server_entry.val);
    #ifdef DEBUG_SEL_CIRCUIT
        print_share(cmp, format("equality {}", i));
    #endif
        return cmp;
  }

};

unique_ptr<CircuitBuilderBase> make_unique_circuit_builder(const CircuitConfig& cfg,
    BooleanCircuit* bcirc, BooleanCircuit* ccirc, ArithmeticCircuit* acirc) {
  if (cfg.use_conversion) {
    return make_unique<CircuitBuilderBase>(
        new CircuitBuilder<ArithShare>(cfg, bcirc, ccirc, acirc));
  } else {
    return make_unique<CircuitBuilderBase>(
        new CircuitBuilder<BoolShare>(cfg, bcirc, ccirc, acirc));
  }
}

} /* end of namespace: sel */
