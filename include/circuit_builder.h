/**
 \file    circuit_builder.h
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

#ifndef SEL_CIRCUIT_BUILDER_H
#define SEL_CIRCUIT_BUILDER_H
#pragma once

#include "fmt/format.h"
using fmt::format;
#include "util.h"
#include "math.h"
#include "seltypes.h"
#include "logger.h"
#include "circuit_input.h"
#include "aby/Share.h"
#include "aby/gadgets.h"

namespace sel {

/**
  * Return type of weight_compare_*()
  * fw - field weight = weight * comparison * empty-deltas
  * w - weight for weight sum = weight * empyt-deltas
  */
struct FieldWeight { ArithShare fw, w; };

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

class CircuitBuilder {
public:
  CircuitBuilder(CircuitConfig cfg_,
    BooleanCircuit* bcirc, BooleanCircuit* ccirc, ArithmeticCircuit* acirc);

  template<class EpilinkInput>
  void set_input(const EpilinkInput& input) {
    ins.set(input);
  }

#ifdef DEBUG_SEL_CIRCUIT
  void set_both_inputs(const EpilinkClientInput& in_client, const EpilinkServerInput& in_server) {
    ins.set_both(in_client, in_server);
  }
#endif


  std::vector<LinkageOutputShares> build_linkage_circuit();

  CountOutputShares build_count_circuit();

  void reset();

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
  BoolShare to_bool(const ArithShare& s);

  ArithShare to_arith(const BoolShare& s);

  BoolShare to_gmw(const BoolShare& s);

  // closures
  const A2BConverter to_bool_closure;
  const B2AConverter to_arith_closure;

  /*
  * Builds the record linkage component of the circuit
  */
  LinkageShares build_single_linkage_circuit(size_t index);

  LinkageOutputShares to_linkage_output(const LinkageShares& s);

  CountOutputShares sum_linkage_shares(std::vector<LinkageShares> ls);

  FieldWeight best_group_weight(size_t index, const IndexSet& group_set);

  /**
   * Cache to store calls to field_weight()
   * Can save half the circuit in permutation groups this way.
   */
  std::map<ComparisonIndex, FieldWeight> field_weight_cache;

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
  const FieldWeight& field_weight(const ComparisonIndex& i);

  /**
  * Calculates dice coefficient of given bitmasks via their hamming weights, up
  * to the configured precision.
  * Note that we use rounding integer division, that is (x+(y/2))/y, because x/y
  * always rounds down, which would lead to a bias.
  */
  BoolShare dice_coefficient(const ComparisonIndex& i);

  /**
  * Binary-compares two shares
  */
  BoolShare equality(const ComparisonIndex& i);

};
} /* end of namespace: sel */

#endif /* end of include guard: SEL_CIRCUIT_BUILDER_H */
