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

#include "circuit_input.h"

class BooleanCircuit;
class ArithmeticCircuit;

namespace sel {

struct LinkageOutputShares {
  OutShare index, match, tmatch;
#ifdef DEBUG_SEL_RESULT
  OutShare score_numerator, score_denominator;
#endif
};

struct CountOutputShares {
  OutShare matches, tmatches;
};

class CircuitBuilderBase {
public:
  virtual ~CircuitBuilderBase() = default;

  virtual void set_input(const EpilinkClientInput& input) = 0;
  virtual void set_input(const EpilinkServerInput& input) = 0;
#ifdef DEBUG_SEL_CIRCUIT
  virtual void set_both_inputs(const EpilinkClientInput& in_client,
      const EpilinkServerInput& in_server) = 0;
#endif

  virtual std::vector<LinkageOutputShares> build_linkage_circuit() = 0;
  virtual CountOutputShares build_count_circuit() = 0;

  virtual void reset() = 0;
};

std::unique_ptr<CircuitBuilderBase> make_unique_circuit_builder(const CircuitConfig& cfg,
    BooleanCircuit* bcirc, BooleanCircuit* ccirc, ArithmeticCircuit* acirc);

} /* end of namespace: sel */

#endif /* end of include guard: SEL_CIRCUIT_BUILDER_H */
