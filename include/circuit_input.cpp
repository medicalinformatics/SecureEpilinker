/**
 \file    circuit_input.cpp
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
 \brief SEL Circuit input helper
*/

#include "fmt/format.h"
using fmt::format;
#include "circuit_input.h"
#include "aby/gadgets.h"
#include "util.h"
#include "logger.h"

using namespace std;

namespace sel {

constexpr auto BIN = FieldComparator::BINARY;
constexpr auto BM = FieldComparator::DICE;

bool operator<(const ComparisonIndex& l, const ComparisonIndex& r) {
  return l.left_idx < r.left_idx
    || (l.left_idx == r.left_idx && l.left < r.left)
    || (l.left_idx == r.left_idx && l.left == r.left && l.right < r.right);
}

CircuitInput::CircuitInput(const CircuitConfig& cfg,
    BooleanCircuit* bcirc, ArithmeticCircuit* acirc) :
  cfg{cfg}, bcirc{bcirc}, acirc{acirc} {}

void CircuitInput::set(const EpilinkClientInput& input) {
  assert(!input_set && "Input already set. Call clear() first if resetting.");
  set_constants(input.database_size, input.num_records);
  set_real_client_input(input);
  set_dummy_server_input();
  get_logger()->trace("SELCircuit inputs set (client only).");
  input_set = true;
}

void CircuitInput::set(const EpilinkServerInput& input) {
  assert(!input_set && "Input already set. Call clear() first if resetting.");
  set_constants(input.database_size, input.num_records);
  set_dummy_client_input();
  set_real_server_input(input);
  get_logger()->trace("SELCircuit inputs set (server only).");
  input_set = true;
}

#ifdef DEBUG_SEL_CIRCUIT
void CircuitInput::set_both(const EpilinkClientInput& in_client,
    const EpilinkServerInput& in_server) {
  assert(!input_set && "Input already set. Call clear() first if resetting.");
  assert(in_client.database_size == in_server.database_size && "Database sizes don't match!");
  assert(in_client.num_records == in_server.num_records && "Number of records don't match!");

  set_constants(in_client.database_size, in_client.num_records);
  set_real_client_input(in_client);
  set_real_server_input(in_server);
  get_logger()->trace("SELCircuit inputs set (both).");
  input_set = true;
}
#endif

void CircuitInput::clear() {
  left_shares.clear();
  right_shares.clear();
  weight_cache.clear();
  dbsize_ = 0;
  nrecords_ = 0;
  input_set = false;
}

ComparisonShares CircuitInput::get(const ComparisonIndex& i) const {
  return {left_shares.at(i.left)[i.left_idx], right_shares.at(i.right)};
}

const ArithShare& CircuitInput::get_const_weight(const ComparisonIndex& i) const {
  FieldNamePair ipair{i.left, i.right};
  const auto& cache_hit = weight_cache.find(ipair);
  if (cache_hit != weight_cache.cend()) {
    get_logger()->trace("weight cache hit for ({}|{})", i.left, i.right);
    return cache_hit->second;
  }

  const CircUnit weight_r = cfg.rescaled_weight(i.left, i.right);
  ArithShare weight = constant_simd(acirc, weight_r, BitLen, dbsize_);

  return weight_cache[ipair] = weight;
}

void CircuitInput::set_constants(size_t database_size, size_t num_records) {
  dbsize_ = database_size;
  nrecords_ = num_records;
  const_idx_ = ascending_numbers_constant(bcirc, dbsize_);

  const_dice_prec_factor_ =
    constant_simd(acirc, (1 << cfg.dice_prec), BitLen, database_size);

  CircUnit T = llround(cfg.epi.threshold * (1 << cfg.dice_prec));
  CircUnit Tt = llround(cfg.epi.tthreshold * (1 << cfg.dice_prec));

  get_logger()->debug(
      "Rescaled threshold: {:x}/ tentative: {:x}", T, Tt);

  const_threshold_ = constant(acirc, T, BitLen);
  const_tthreshold_ = constant(acirc, Tt, BitLen);
#ifdef DEBUG_SEL_CIRCUIT
  print_share(const_idx_, "const_idx");
  print_share(const_dice_prec_factor_, "const_dice_prec_factor");
  print_share(const_threshold_, "const_threshold ");
  print_share(const_tthreshold_, "const_tthreshold ");
#endif
}

void CircuitInput::set_real_client_input(const EpilinkClientInput& input) {
  for (const auto& _f : cfg.epi.fields) {
    const FieldName& i = _f.first;
    left_shares[i] = make_client_entry_shares(input, i);
  }
}

void CircuitInput::set_real_server_input(const EpilinkServerInput& input) {
  for (const auto& _f : cfg.epi.fields) {
    const FieldName& i = _f.first;
    right_shares[i] = make_server_entries_share(input, i);
  }
}

void CircuitInput::set_dummy_client_input() {
  for (const auto& _f : cfg.epi.fields) {
    const FieldName& i = _f.first;
    auto& entries = left_shares[i];
    entries.reserve(nrecords_);
    for (size_t j = 0; j != nrecords_; ++j) {
      entries.emplace_back(make_dummy_entry_share(i));
    }
  }
}

void CircuitInput::set_dummy_server_input() {
  for (const auto& _f : cfg.epi.fields) {
    const FieldName& i = _f.first;
    right_shares[i] = make_dummy_entry_share(i);
  }
}

EntryShare CircuitInput::make_server_entries_share(const EpilinkServerInput& input,
    const FieldName& i) {
  const auto& f = cfg.epi.fields.at(i);
  const VFieldEntry& entries = input.database->at(i);
  size_t bytesize = bitbytes(f.bitsize);
  Bitmask dummy_bm(bytesize);
  VBitmask values = transform_vec(entries,
      [&dummy_bm](auto e){return e.value_or(dummy_bm);});
  check_vectors_size(values, bytesize, "server input byte vector "s + i);

  // value
  BoolShare val(bcirc,
      concat_vec(values).data(), f.bitsize, SERVER, dbsize_);

  // delta
  vector<CircUnit> db_delta(dbsize_);
  for (size_t j=0; j!=dbsize_; ++j) db_delta[j] = entries[j].has_value();
  ArithShare delta(acirc, db_delta.data(), BitLen, SERVER, dbsize_);

  // Set hammingweight input share only for bitmasks
  BoolShare _hw;
  if (f.comparator == BM) {
    auto value_hws = transform_vec(values, hw);
    _hw = BoolShare(bcirc,
        value_hws.data(), hw_size(f.bitsize), SERVER, dbsize_);
  }

#ifdef DEBUG_SEL_CIRCUIT
    print_share(val, format("server val[{}]", i));
    print_share(delta, format("server delta[{}]", i));
    if (f.comparator == BM) print_share(_hw, format("server hw[{}]", i));
#endif

  return {val, delta, _hw};

}

VEntryShare CircuitInput::make_client_entry_shares(const EpilinkClientInput& input,
    const FieldName& i) {
  VEntryShare entry_shares;
  entry_shares.reserve(nrecords_);
  for (size_t j = 0; j != nrecords_; ++j) {
    entry_shares.emplace_back(make_client_entry_share(input, i, j));
  }
  return entry_shares;
}

EntryShare CircuitInput::make_client_entry_share(const EpilinkClientInput& input,
    const FieldName& i, size_t index) {
  const auto& f = cfg.epi.fields.at(i);
  const FieldEntry& entry = input.records->at(index).at(i);
  size_t bytesize = bitbytes(f.bitsize);
  Bitmask value = entry.value_or(Bitmask(bytesize));
  check_vector_size(value, bytesize, "client input byte vector "s + i);

  // value
  BoolShare val(bcirc,
      repeat_vec(value, dbsize_).data(),
      f.bitsize, CLIENT, dbsize_);

  // delta
  ArithShare delta(acirc,
      vector<CircUnit>(dbsize_, static_cast<CircUnit>(entry.has_value())).data(),
      BitLen, CLIENT, dbsize_);

  // Set hammingweight input share only for bitmasks
  BoolShare _hw;
  if (f.comparator == BM) {
    _hw = BoolShare(bcirc,
        vector<CircUnit>(dbsize_, hw(value)).data(),
        hw_size(f.bitsize), CLIENT, dbsize_);
  }

#ifdef DEBUG_SEL_CIRCUIT
    print_share(val, format("client[{}] val[{}]", index, i));
    print_share(delta, format("client[{}] delta[{}]", index, i));
    if (f.comparator == BM) print_share(_hw, format("client[{}] hw[{}]", index, i));
#endif

  return {val, delta, _hw};
}

EntryShare CircuitInput::make_dummy_entry_share(const FieldName& i) {
  const auto& f = cfg.epi.fields.at(i);

  BoolShare val(bcirc, f.bitsize, dbsize_); //dummy val

  ArithShare delta(acirc, BitLen, dbsize_); // dummy delta

  BoolShare _hw;
  if (f.comparator == BM) {
    _hw = BoolShare(bcirc, hw_size(f.bitsize), dbsize_); //dummy hw
  }

#ifdef DEBUG_SEL_CIRCUIT
    print_share(val, format("dummy val[{}]", i));
    print_share(delta, format("dummy delta[{}]", i));
    if (f.comparator == BM) print_share(_hw, format("dummy hw[{}]", i));
#endif

  return {val, delta, _hw};
}

} /* end of namespace: sel */
