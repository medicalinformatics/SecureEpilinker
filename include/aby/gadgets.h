/**
 \file    sel/aby/gadgets.h
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
 \brief ABY circuit gadgets
*/

#ifndef SEL_ABY_GADGETS_H
#define SEL_ABY_GADGETS_H
#pragma once

#include <functional>
#include "Share.h"

namespace sel {

template <class ShareT> struct Quotient { ShareT num, den; };
using ArithQuotient = Quotient<ArithShare>;
using BoolQuotient = Quotient<BoolShare>;

template <class ShareT>
  using UnaryOp = std::function<ShareT (const ShareT&)>;

template <class ShareT>
  using BinaryOp = std::function<ShareT (const ShareT&, const ShareT&)>;
template <class ShareT>
  using QuotientSelector = std::function<BoolShare (const Quotient<ShareT>&, const Quotient<ShareT>&)>;

template <class ShareT>
  using T2BConverter = std::function<BoolShare (const ShareT&)>;
using A2BConverter = std::function<BoolShare (const ArithShare&)>;
template <class ShareT>
  using B2TConverter = std::function<ShareT (const BoolShare&)>;
using B2AConverter = std::function<ArithShare (const BoolShare&)>;

template<class ShareT>
void print_share(const Quotient<ShareT>& q, const std::string& msg);

/**
 * Accumulates all values in simd_share as if it was first split and then
 * binary-accumulated using op. Returns a share of same bitlen and nvals=1.
 *
 * More efficient than binary accumulate because share is repetitively split and
 * then op applied to only two values, effectively dividing nvals by 2 in each
 * iteration. If nvals is odd, remainder share is saved and later appended if
 * there is another odd split.
 */
BoolShare split_accumulate(BoolShare simd_share, const BinaryOp<BoolShare>& op);

/**
 * Like split_accumulate but more specific to running a selector op_select which
 * returns a share with only one wire. This share is then muxed to select either
 * the first or second part of a split share. The same selection process is also
 * applied in parallel to target.
 * Output will be saved to input references, so initial shares are overwritten.
 */
void split_select_target(BoolShare& selector, BoolShare& target,
    const BinaryOp<BoolShare>& op_select);

template <class ShareT>
QuotientSelector<ShareT> make_max_selector(const T2BConverter<ShareT>& to_bool);
template <class ShareT>
QuotientSelector<ShareT> make_min_selector(const T2BConverter<ShareT>& to_bool);

template <class ShareT>
QuotientSelector<ShareT> make_max_tie_selector(const T2BConverter<ShareT>& to_bool,
    const size_t den_bits = 0);
template <class ShareT>
QuotientSelector<ShareT> make_min_tie_selector(const T2BConverter<ShareT>& to_bool,
    const size_t den_bits = 0);

template <class ShareT>
ShareT sum(const std::vector<ShareT>&);

BoolQuotient max_tie(const std::vector<BoolQuotient>& qs);

ArithQuotient max_tie(const std::vector<ArithQuotient>& qs,
    const A2BConverter& to_bool, const B2AConverter& to_arith,
    const size_t den_bits = 0);

BoolShare max(const std::vector<BoolShare>&);

BoolQuotient max(const ArithQuotient& a, const ArithQuotient& b,
    const A2BConverter& to_bool);
ArithQuotient max(const ArithQuotient& a, const ArithQuotient& b,
    const A2BConverter& to_bool, const B2AConverter& to_arith);
ArithQuotient max(const std::vector<ArithQuotient>& qs,
    const A2BConverter& to_bool, const B2AConverter& to_arith);

BoolShare ascending_numbers_constant(BooleanCircuit* bcirc,
    size_t nvals, size_t start = 0);

} // namespace sel
#endif /* end of include guard: SEL_ABY_GADGETS_H */
