/**
 \file    sel/aby/Share.cpp
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
 \brief ABY share wrapper
*/

#include <fmt/format.h>
#include "Share.h"

namespace sel {

/******************** BoolShare ********************/

BoolShare apply_file_binary(const BoolShare& a, const BoolShare& b,
    uint32_t a_bits, uint32_t b_bits, const string& fn) {
  assert (a.get_nvals() == b.get_nvals());
  vector<uint32_t> a_wires = a.get()->get_wires(), b_wires = b.get()->get_wires();
  assert (a_wires.size() >= a_bits);
  assert (b_wires.size() >= b_bits);

  vector<uint32_t> in;
  in.reserve(a_bits + b_bits);
  copy_n(begin(a_wires), a_bits, back_inserter(in));
  copy_n(begin(b_wires), b_bits, back_inserter(in));

  return BoolShare{a.bcirc, a.bcirc->PutGateFromFile(fn, in, a.get_nvals())};
}

vector<BoolShare> BoolShare::split(uint32_t new_nval) {
  const size_t bitlen{get_bitlen()}, nvals{get_nvals()},
      numshares{ceil_divide(nvals, new_nval)};
  vector<vector<uint32_t>> split_wires;
  split_wires.reserve(bitlen);
  vector<uint32_t> new_nvals(numshares, new_nval);
  if (nvals%new_nval) new_nvals.back() = nvals%new_nval;

  for (uint32_t id : sh.get()->get_wires()) {
    split_wires.emplace_back(bcirc->PutSplitterGate(id, new_nvals));
  }

  // assemble shares
  assert (numshares == split_wires.at(0).size());// all splits should have this size
  vector<BoolShare> res;
  vector<uint32_t> wires(bitlen);
  res.reserve(numshares);
  for (size_t i = 0; i != numshares; ++i) {
    for (size_t j = 0; j != bitlen; j++) {
      wires[j] = split_wires[j][i];
    }
    res.emplace_back(BoolShare(bcirc, new boolshare(wires, bcirc)));
  }

  return res;
}

BoolShare BoolShare::zeropad(uint32_t bitlen) const {
  assert(get_bitlen() <= bitlen);

  vector<uint32_t> wires(sh->get_wires());
  uint32_t zero = bcirc->PutConstantGate(0, get_nvals());
  wires.insert(wires.end(), bitlen-wires.size(), zero);
  return BoolShare{bcirc, wires};
}

/******************** OutShare ********************/

vector<uint32_t> OutShare::get_clear_value_vec() {
  uint32_t* arr;
  uint32_t nvals, bitlen;
  write_clear_value_vec(&arr, &bitlen, &nvals);
  assert(bitlen == 32); // TODO <= 32

  vector<uint32_t> vec(nvals);
  for (size_t i = 0; i != nvals; ++i) {
    // TODO more general case bitlen < 32
    vec[i] = arr[i];
  }

  return vec;
}

/******************** Factories ********************/
/*
 * OutShare factory
 */
OutShare out(const Share& share, e_role dst) {
  return OutShare(share.get_circuit(),
      share.get_circuit()->PutOUTGate(share.get(), dst));
}

/*
 * Debugging PrintValueGate
 */
OutShare print_share(const Share& share, const string& msg) {
  string desc = fmt::format("({},{} {}) ",
      share.get_bitlen(), share.get_nvals(), msg);
  return OutShare(share.get_circuit(),
      share.get_circuit()->PutPrintValueGate(share.get(), desc));
}

Share constant(Circuit* c, UGATE_T val, uint32_t bitlen) {
  return Share{c, c->PutCONSGate(val, bitlen)};
}

BoolShare constant(BooleanCircuit* c, UGATE_T val, uint32_t bitlen) {
  return BoolShare{c, c->PutCONSGate(val, bitlen)};
}

Share constant_simd(Circuit* c, UGATE_T val, uint32_t bitlen, uint32_t nvals) {
  return Share{c, c->PutSIMDCONSGate(nvals, val, bitlen)};
}

BoolShare constant_simd(BooleanCircuit* c, UGATE_T val, uint32_t bitlen, uint32_t nvals){
  return BoolShare{c, c->PutSIMDCONSGate(nvals, val, bitlen)};
}

BoolShare constant_simd(ArithmeticCircuit* c, UGATE_T val, uint32_t bitlen, uint32_t nvals){
  return ArithShare{c, c->PutSIMDCONSGate(nvals, val, bitlen)};
}

/******************** Conversions ********************/
BoolShare a2y(BooleanCircuit* ycirc, const ArithShare& s) {
  return BoolShare{ycirc, ycirc->PutA2YGate(s.get())};
}

ArithShare y2a(ArithmeticCircuit* acirc, BooleanCircuit* bcirc, const BoolShare& s) {
  share* sh = acirc->PutY2AGate(s.get(),
      static_cast<Circuit*>(bcirc));
  return ArithShare{ acirc, sh };
}

BoolShare a2b(BooleanCircuit* bcirc, BooleanCircuit* ycirc, const ArithShare& s) {
  return BoolShare{ bcirc,
    static_cast<Circuit*>(bcirc)->PutA2BGate(s.get(),
      static_cast<Circuit*>(ycirc)) };
}

ArithShare b2a(ArithmeticCircuit* acirc, const BoolShare& s) {
  return ArithShare{acirc, acirc->PutB2AGate(s.get())};
}

BoolShare y2b(BooleanCircuit* bcirc, const BoolShare& s) {
  return BoolShare{bcirc, bcirc->PutY2BGate(s.get())};
}

BoolShare b2y(BooleanCircuit* ycirc, const BoolShare& s) {
  return BoolShare{ycirc, ycirc->PutB2YGate(s.get())};
}

/******************** SIMD stuff ********************/

/**
 * Vertically Combines the given shares to a new share having the same number of
 * wires=bitlen and nvals the sum of the individual nvals
 */
BoolShare vcombine(const vector<BoolShare>& shares) {
  // TODO handle size() == 0 ?
  size_t bitlen = shares.at(0).get_bitlen();
  vector<vector<uint32_t>> combwires{bitlen};
  vector<uint32_t> reswires;
  reswires.reserve(bitlen);
  BooleanCircuit* circ = shares.at(0).get_circuit();
  for (const auto& share : shares) {
    vector<uint32_t> wires{share.get()->get_wires()};
    for (size_t i{0}; i != wires.size(); ++i) {
      combwires[i].emplace_back(wires[i]);
    }
  }
  for (vector<uint32_t>& cw : combwires) {
    reswires.emplace_back(circ->PutCombinerGate(cw));
  }
  return BoolShare{circ, reswires};
}

// TODO sum, all, any, prod -> gadgets
} // namespace sel
