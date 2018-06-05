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
#include "abycore/aby/abyparty.h"
#include "Share.h"

namespace sel {

using UnaryOp_Share = std::function<Share (const Share&)>;
using UnaryOp_BoolShare = std::function<BoolShare (const BoolShare&)>;
using UnaryOp_ArithShare = std::function<ArithShare (const ArithShare&)>;
using BinaryOp_Share = std::function<Share (const Share&, const Share&)>;
using BinaryOp_BoolShare = std::function<BoolShare (const BoolShare&, const BoolShare&)>;
using BinaryOp_ArithShare = std::function<ArithShare (const ArithShare&, const ArithShare&)>;

/*
Share binary_accumulate(vector<Share> vals,
    const BinaryOp_Share& op);

BoolShare binary_accumulate(vector<BoolShare> vals,
    const BinaryOp_BoolShare& op);

ArithShare binary_accumulate(vector<ArithShare> vals,
    const BinaryOp_ArithShare& op);
*/

BoolShare split_accumulate(BoolShare simd_share,
    const BinaryOp_BoolShare& op);

BoolShare max(const vector<BoolShare>&);
BoolShare sum(const vector<BoolShare>&);
ArithShare sum(const vector<ArithShare>&);

} // namespace sel
#endif /* end of include guard: SEL_ABY_GADGETS_H */
