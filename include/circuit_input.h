/**
 \file    circuit_input.h
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

#ifndef SEL_CIRCUIT_INPUT_H
#define SEL_CIRCUIT_INPUT_H
#pragma once

#include "aby/Share.h"
#include "seltypes.h"
#include "circuit_config.h"

namespace sel {

/**
  * An EntryShare holds the value of the field itself, a delta flag, which is 1
  * if the field is non-empty, and the precalculated hammingweight for bitmasks.
  */
template <class MultShare>
struct EntryShare {
  BoolShare val; // value as bool
  MultShare delta; // 1 if non-empty, 0 o/w
  BoolShare hw; // precomputed HW of val - not used for bin
};

template <class MultShare>
using VEntryShare = std::vector<EntryShare<MultShare>>;

template <class MultShare>
struct ComparisonShares {
  const EntryShare<MultShare> &left, &right;
};

struct ComparisonIndex {
  size_t left_idx;
  const FieldName left, right;
};

// We want to use ComparisonIndex as map keys
bool operator<(const ComparisonIndex& l, const ComparisonIndex& r);

using FieldNamePair = std::pair<FieldName, FieldName>;

template <class MultShare>
class CircuitInput {
  public:
    CircuitInput(const CircuitConfig& cfg,
        BooleanCircuit* bcirc, ArithmeticCircuit* acirc);
    void set(const EpilinkClientInput& input);
    void set(const EpilinkServerInput& input);
#ifdef DEBUG_SEL_CIRCUIT
    void set_both(const EpilinkClientInput& in_client,
        const EpilinkServerInput& in_server);
#endif
    void clear();

    bool is_input_set() const { return input_set; }
    size_t dbsize() const { return dbsize_; }
    size_t nrecords() const { return nrecords_; }
    ComparisonShares<MultShare> get(const ComparisonIndex& i) const;
    const MultShare& get_const_weight(const ComparisonIndex& i) const;
    const BoolShare& const_idx() const { return const_idx_; }
    const MultShare& const_dice_prec_factor() const { return const_dice_prec_factor_; }
    const MultShare& const_threshold() const { return const_threshold_; }
    const MultShare& const_tthreshold() const { return const_tthreshold_; }

  private:
    static constexpr bool do_arith_mult = std::is_same_v<MultShare, ArithShare>;
    static constexpr size_t delta_bitlen = do_arith_mult ? BitLen : 1;
    using MultCircuit = std::conditional_t<do_arith_mult, ArithmeticCircuit, BooleanCircuit>;

    const CircuitConfig& cfg;
    BooleanCircuit* bcirc;
    ArithmeticCircuit* acirc;
    MultCircuit* mcirc;
    bool input_set{false};

    size_t dbsize_{0};
    size_t nrecords_{0};
    // Constant shares
    BoolShare const_idx_;
    MultShare const_dice_prec_factor_;
    // Left side of inequality: T * sum(weights)
    MultShare const_threshold_, const_tthreshold_;
    mutable std::map<FieldNamePair, MultShare> weight_cache;

    std::map<FieldName, VEntryShare<MultShare>> left_shares;
    std::map<FieldName, EntryShare<MultShare>> right_shares;

    void set_constants(size_t database_size, size_t num_records);
    void set_real_client_input(const EpilinkClientInput& input);
    void set_real_server_input(const EpilinkServerInput& input);
    void set_dummy_client_input();
    void set_dummy_server_input();
    EntryShare<MultShare> make_server_entries_share(const EpilinkServerInput& input,
        const FieldName& i);
    VEntryShare<MultShare> make_client_entry_shares(const EpilinkClientInput& input,
        const FieldName& i);
    EntryShare<MultShare> make_client_entry_share(const EpilinkClientInput& input,
        const FieldName& i, size_t index);
    EntryShare<MultShare> make_dummy_entry_share(const FieldName& i);
};

} /* end of namespace: sel */

namespace fmt {

template <>
struct formatter<sel::ComparisonIndex> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const sel::ComparisonIndex& i, FormatContext &ctx) {
    return format_to(ctx.begin(),"[{}]({}|{})", i.left_idx, i.left, i.right);
  }
};

} /* end of namespace: fmt */

#endif /* end of include guard: SEL_CIRCUIT_INPUT_H */
