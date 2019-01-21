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
#ifdef SEL_STATS
#include "aby/statsprinter.h"
#endif

// ABY forward declarations
class BooleanCircuit;
class ArithmeticCircuit;
class ABYParty;

namespace sel {

enum class MPCRole { CLIENT, SERVER };

class CircuitBuilderBase; // forward declaration of circuit builder class

class SecureEpilinker {
public:
  struct ABYConfig {
    MPCRole role;
    std::string host; // local for role SERVER, remote for role CLIENT
    uint16_t port;
    uint32_t nthreads;
  };

  struct State {
    size_t num_records{0};
    size_t database_size{0};
    bool built{false};
    bool matching_mode{false};
    bool setup{false};
    bool input_set{false};
  protected:
    friend class SecureEpilinker;
    void reset();
  };

  SecureEpilinker(ABYConfig aby_config, CircuitConfig circuit_config);
  ~SecureEpilinker();

  /**
   * Opens the network connection of two SecureEpilinkers as specified in the
   * ABY configuration. This is a blocking call.
   */
  void connect();

  void build_linkage_circuit(const size_t num_records, const size_t database_size);
  void build_count_circuit(const size_t num_records, const size_t database_size);

  /*
   * TODO The separation of setup and online phase is currently not possible in
   * ABY. Until it is, this will all happen in the run_* methods...
   */
  void run_setup_phase();

  /**
   * Interactively runs the circuit with given inputs and returns max index as
   * XOR share to be sent to linkage service by the caller.
   * database size must match on both sides and be smaller than used nvals
   * during build_circuit()
   */
  void set_client_input(const EpilinkClientInput& input);
  void set_server_input(const EpilinkServerInput& input);
#ifdef DEBUG_SEL_CIRCUIT
  void set_both_inputs(const EpilinkClientInput& in_client,
      const EpilinkServerInput& in_server);
#endif

  /**
   * Multiple dispatch versions of set_*_input()
   */
  void set_input(const EpilinkClientInput& input) { return set_client_input(input); }
  void set_input(const EpilinkServerInput& input) { return set_server_input(input); }

  std::vector<Result<CircUnit>> run_linkage();
  CountResult<CircUnit> run_count();

  /**
   * Resets the ABY Party and states.
   */
  void reset();

  State get_state();

#ifdef SEL_STATS
  sel::aby::StatsPrinter get_stats_printer();
#endif

private:
  std::unique_ptr<ABYParty> party;
  BooleanCircuit* bcirc; // boolean circuit for boolean parts
  BooleanCircuit* ccirc; // intermediate conversion circuit
  ArithmeticCircuit* acirc;
  const CircuitConfig cfg;

  std::unique_ptr<CircuitBuilderBase> selc; // ~pimpl

  /*
   * Note that we currently maintain an outside-facing state that behaves as if
   * aby already has the separation of circuit building/setup/input setting.
   * The SELCircuit has its own real state. Hence, once properly implemented on
   * aby's side, we can delete this public fake state.
   */
  State state;

  /*
   * TODO It is currently not possible to build an ABY circuit without
   * specifying the inputs, as all circuits start with the InputGates. Hence,
   * the actual circuit building will happen in run_*.
   */
  void build_circuit(const size_t num_records, const size_t database_size);
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
        ((conf.role == sel::MPCRole::SERVER) ? "Server" : "Client"),
        ((conf.role == sel::MPCRole::SERVER) ? "binding to" : "remote host"),
        conf.host, conf.port, conf.nthreads);
  }
};

} // namespace fmt

#endif /* end of include guard: SEL_SECURE_EPILINKER_H */
