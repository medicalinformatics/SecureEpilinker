/**
 \file    secure_epilinker.h
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

#ifndef SEL_SECURE_EPILINKER_H
#define SEL_SECURE_EPILINKER_H
#pragma once

#include "epilink_input.h"
#include "abycore/aby/abyparty.h"

namespace sel {

class SecureEpilinker {
public:
  struct ABYConfig {
    e_role role; // SERVER or CLIENT
    e_sharing bool_sharing; // S_YAO or S_BOOL for boolean circuit parts?
    std::string remote_host;
    uint16_t port;
    uint32_t nthreads;
  };

  /**
   * Result of the secure epilink protocol as returend by run_as_*()
   * Fields hold XOR shares to be assembled by the LinkageService
   *
   * If DEBUG_SEL_RESULT is set, fields hold plain text values and the numerator
   * and denominator of the score.
   */
  struct Result {
    CircUnit index; // XOR share of matching index
    bool match; // XOR share of match bit
    bool tmatch; // XOR share of tentative match bit
#ifdef DEBUG_SEL_RESULT
    CircUnit score_numerator, score_denominator;
#endif
  };

  SecureEpilinker(ABYConfig aby_config, EpilinkConfig epi_config);
  ~SecureEpilinker();

  /*
   * TODO It is currently not possible to build an ABY circuit without
   * specifying the inputs, as all circuits start with the InputGates. Hence,
   * the actual circuit building will happen in run_as_*.
   */
  void build_circuit(const uint32_t nvals);

  /*
   * TODO The separation of setup and online phase is currently not possible in
   * ABY. Until it is, this will all happen in the run_as_* methods...
   */
  void run_setup_phase();

  /**
   * Interactively runs the circuit with given inputs and returns max index as
   * XOR share to be sent to linkage service by the caller.
   * database size must match on both sides and be smaller than used nvals
   * during build_circuit()
   */
  Result run_as_client(const EpilinkClientInput& input);
  Result run_as_server(const EpilinkServerInput& input);
#ifdef DEBUG_SEL_CIRCUIT
  Result run_as_both(const EpilinkClientInput& in_client,
      const EpilinkServerInput& in_server);
#endif

  /**
   * Shortcut for run_as_*() depending on input
   */
  Result run(const EpilinkClientInput& input) { return run_as_client(input); }
  Result run(const EpilinkServerInput& input) { return run_as_server(input); }

  /**
   * Resets the ABY Party and states.
   */
  void reset();

private:
  ABYParty party;
  BooleanCircuit* bcirc; // boolean circuit for boolean parts
  BooleanCircuit* ccirc; // intermediate conversion circuit
  ArithmeticCircuit* acirc;
  const EpilinkConfig epicfg;

  class SELCircuit; // forward of implementation class ~ pimpl
  unique_ptr<SELCircuit> selc; // ~pimpl

  bool is_built{false};
  bool is_setup{false};

  // called by run_as_*() after inputs are set
  Result run_circuit();
};

} // namespace sel
#endif /* end of include guard: SEL_SECURE_EPILINKER_H */
