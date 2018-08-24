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

struct ArithQuotient { ArithShare num, den; };
struct BoolQuotient { BoolShare num, den; };

using UnaryOp_Share = std::function<Share (const Share&)>;
using UnaryOp_BoolShare = std::function<BoolShare (const BoolShare&)>;
using UnaryOp_ArithShare = std::function<ArithShare (const ArithShare&)>;
using BinaryOp_Share = std::function<Share (const Share&, const Share&)>;
using BinaryOp_BoolShare = std::function<BoolShare (const BoolShare&, const BoolShare&)>;
using BinaryOp_ArithShare = std::function<ArithShare (const ArithShare&, const ArithShare&)>;
using BinaryOp_ArithQuotient = std::function<ArithQuotient (const ArithQuotient&, const ArithQuotient&)>;
using ArithQuotientSelector = std::function<BoolShare (const ArithQuotient&, const ArithQuotient&)>;

using A2BConverter = std::function<BoolShare (const ArithShare&)>;
using B2AConverter = std::function<ArithShare (const BoolShare&)>;

/*
Share binary_accumulate(vector<Share> vals,
    const BinaryOp_Share& op);

BoolShare binary_accumulate(vector<BoolShare> vals,
    const BinaryOp_BoolShare& op);

ArithShare binary_accumulate(vector<ArithShare> vals,
    const BinaryOp_ArithShare& op);
*/

void print_share(const ArithQuotient& q, const std::string& msg);
void print_share(const BoolQuotient& q, const std::string& msg);

/**
 * Accumulates all values in simd_share as if it was first split and then
 * binary-accumulated using op. Returns a share of same bitlen and nvals=1.
 *
 * More efficient than binary accumulate because share is repetitively split and
 * then op applied to only two values, effectively dividing nvals by 2 in each
 * iteration. If nvals is odd, remainder share is saved and later appended if
 * there is another odd split.
 */
BoolShare split_accumulate(BoolShare simd_share, const BinaryOp_BoolShare& op);

/**
 * Like split_accumulate but more specific to running a selector op_select which
 * returns a share with only one wire. This share is then muxed to select either
 * the first or second part of a split share. The same selection process is also
 * applied in parallel to target.
 * Output will be saved to input references, so initial shares are overwritten.
 */
void split_select_target(BoolShare& selector, BoolShare& target,
    const BinaryOp_BoolShare& op_select);

void split_select_quotient_target(
    ArithQuotient& selector, std::vector<BoolShare>& targets,
    const ArithQuotientSelector& op_select, const B2AConverter& to_arith);

ArithQuotientSelector make_max_selector(const A2BConverter& to_bool);
ArithQuotientSelector make_min_selector(const A2BConverter& to_bool);

void max_index(
    ArithQuotient& selector, std::vector<BoolShare>& targets,
    const A2BConverter& to_bool, const B2AConverter& to_arith);

void min_index(
    ArithQuotient& selector, std::vector<BoolShare>& targets,
    const A2BConverter& to_bool, const B2AConverter& to_arith);

BoolShare max(const std::vector<BoolShare>&);
BoolShare sum(const std::vector<BoolShare>&);
ArithShare sum(const std::vector<ArithShare>&);

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
