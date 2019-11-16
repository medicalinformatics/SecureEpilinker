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
#include "fmt/format.h"
using fmt::format;
#include "abycore/aby/abyparty.h"
#include "abycore/sharing/sharing.h"
#include "secure_epilinker.h"
#include "circuit_builder.h"
#include "util.h"
#include "seltypes.h"
#include "logger.h"

using namespace std;

namespace sel {

using State = SecureEpilinker::State;
constexpr auto GMW = BooleanSharing::GMW;
constexpr auto YAO = BooleanSharing::YAO;

/******************** Public Epilinker Interface ********************/
e_sharing to_aby_sharing(BooleanSharing s) {
  return s == GMW ? S_BOOL : S_YAO;
}

e_role to_aby_role(MPCRole r) {
  return r == MPCRole::CLIENT ? CLIENT : SERVER;
}

SecureEpilinker::SecureEpilinker(ABYConfig config, CircuitConfig circuit_config) :
  party{make_unique<ABYParty>(to_aby_role(config.role), config.host, config.port, LT, BitLen, config.nthreads)},
  bcirc{dynamic_cast<BooleanCircuit*>(party->GetSharings()[to_aby_sharing(circuit_config.bool_sharing)]
      ->GetCircuitBuildRoutine())},
  ccirc{dynamic_cast<BooleanCircuit*>(party->GetSharings()[to_aby_sharing(other(circuit_config.bool_sharing))]
      ->GetCircuitBuildRoutine())},
  acirc{dynamic_cast<ArithmeticCircuit*>(party->GetSharings()[S_ARITH]->GetCircuitBuildRoutine())},
  cfg{circuit_config}, selc{make_unique_circuit_builder(cfg, bcirc, ccirc, acirc)} {
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
  party->ConnectAndBaseOTs();
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

Result<CircUnit> to_clear_value(LinkageOutputShares& res, [[maybe_unused]] size_t dice_prec) {
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
  party->ExecCircuit();
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
  party->ExecCircuit();
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
  party->Reset();
  state.reset();
}

#ifdef SEL_STATS
sel::aby::StatsPrinter SecureEpilinker::get_stats_printer() {
  return sel::aby::StatsPrinter(*party);
}
#endif

} // namespace sel
