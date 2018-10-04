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

#include "fmt/format.h"
#include "epilink_input.h"
#include "epilink_result.hpp"
#include "circuit_config.h"
#include "abycore/aby/abyparty.h"

class BooleanCircuit;
class ArithmeticCircuit;

namespace sel {

class SecureEpilinker {
public:
  struct ABYConfig {
    e_role role; // SERVER or CLIENT
    e_sharing bool_sharing; // S_YAO or S_BOOL for boolean circuit parts?
    std::string host; // local for role SERVER, remote for role CLIENT
    uint16_t port;
    uint32_t nthreads;
  };

  SecureEpilinker(ABYConfig aby_config, CircuitConfig circuit_config);
  ~SecureEpilinker();

  /**
   * Opens the network connection of two SecureEpilinkers as specified in the
   * ABY configuration. This is a blocking call.
   */
  void connect();

  /*
   * TODO It is currently not possible to build an ABY circuit without
   * specifying the inputs, as all circuits start with the InputGates. Hence,
   * the actual circuit building will happen in run_as_*.
   */
  void build_circuit(const uint32_t database_size, const uint32_t num_records);

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
  Result<CircUnit> run_as_client(std::unique_ptr<EpilinkClientInput>&& input);
  Result<CircUnit> run_as_server(const std::shared_ptr<EpilinkServerInput>& input);
#ifdef DEBUG_SEL_CIRCUIT
  Result<CircUnit> run_as_both(const EpilinkClientInput& in_client,
      const EpilinkServerInput& in_server);
#endif

  /**
   * Shortcut for run_as_*() depending on input
   */
  Result<CircUnit> run(const EpilinkClientInput& input) { return run_as_client(input); }
  Result<CircUnit> run(const EpilinkServerInput& input) { return run_as_server(input); }

  /**
   * Resets the ABY Party and states.
   */
  void reset();

private:
  ABYParty party;
  BooleanCircuit* bcirc; // boolean circuit for boolean parts
  BooleanCircuit* ccirc; // intermediate conversion circuit
  ArithmeticCircuit* acirc;
  const CircuitConfig cfg;

  class SELCircuit; // forward of implementation class ~ pimpl
  std::unique_ptr<SELCircuit> selc; // ~pimpl

  bool is_built{false};
  bool is_setup{false};

  // called by run_as_*() after inputs are set
  Result<CircUnit> run_circuit();
};

} // namespace sel

namespace fmt {

template <>
struct formatter<sel::SecureEpilinker::ABYConfig> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const sel::SecureEpilinker::ABYConfig& conf, FormatContext &ctx) {
    return format_to(ctx.begin(),
        "ABYConfig{{role={}, sharing={}, {}={}:{}, threads={}}}",
        ((conf.role == SERVER) ? "Server" : "Client"),
        ((conf.bool_sharing == S_YAO) ? "Yao" : "GMW"),
        ((conf.role == SERVER) ? "binding to" : "remote host"),
        conf.host, conf.port, conf.nthreads);
  }
};

} // namespace fmt

#endif /* end of include guard: SEL_SECURE_EPILINKER_H */
