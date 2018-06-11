/**
 \file    sel/aby/Share.h
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

#ifndef SEL_ABY_SHARE_H
#define SEL_ABY_SHARE_H
#pragma once

#include <memory>
#include "abycore/aby/abyparty.h"
#include "circuit_defs.h"

namespace sel {

class Share {
  public:
  // nullptr constructor
  Share(): circ{}, sh{} {};
  /*
   * Constructor to wrap a share object with its circuit
   */
  Share(Circuit* c, share* s) : circ{c}, sh{s} {};
  Share(Circuit* c, const share_p& sp) : circ{c}, sh{sp} {};
  ~Share() = default;
  /*
   * Constructor to create new INGate from plain-text value
   */
  template<typename T>
    // require T of type uintXX_t or uintXX_t*
  Share(Circuit* c, T value, uint32_t bitlen, e_role role) :
    circ{c}, sh{circ->PutINGate(value, bitlen, role)} {}
    /*
  Share(Circuit* c, uint8_t value, uint32_t bitlen, e_role role) :
    circ{c}, sh{c->PutINGate(value, bitlen, role)} {}
  Share(Circuit* c, uint16_t value, uint32_t bitlen, e_role role) :
    circ{c}, sh{c->PutINGate(value, bitlen, role)} {}
  Share(Circuit* c, uint32_t value, uint32_t bitlen, e_role role) :
    circ{c}, sh{c->PutINGate(value, bitlen, role)} {}
  Share(Circuit* c, uint64_t value, uint32_t bitlen, e_role role) :
    circ{c}, sh{c->PutINGate(value, bitlen, role)} {}
    */
  // TODO copy those constructors without role to make SharedINGate

  /*
   * Constructor to create new SIMDINGate from plain-text value
   */
  template<typename T>
    // require T of type uintXX_t or uintXX_t*
  Share(Circuit* c, T value, uint32_t bitlen, e_role role, uint32_t nvals) :
    circ{c}, sh{circ->PutSIMDINGate(nvals, value, bitlen, role)} {}

  /*
   * DummyINGate
   */
  Share(Circuit* circ, uint32_t bitlen) :
    circ{circ}, sh{circ->PutDummyINGate(bitlen)} {}

  /*
   * DummySIMDINGate
   */
  Share(Circuit* circ, uint32_t bitlen, uint32_t nvals) :
    circ{circ}, sh{circ->PutDummySIMDINGate(nvals, bitlen)} {}

  Share& operator+=(const Share& other) {
    assert(circ == other.circ);
    sh = share_p{circ->PutADDGate(sh.get(), other.sh.get())};
    return *this;
  }

  Share& operator*=(const Share& other) {
    assert(circ == other.circ);
    sh = share_p{circ->PutMULGate(sh.get(), other.sh.get())};
    return *this;
  }

  friend Share operator+(Share me, const Share& other) {
    me += other;
    return me;
  }

  friend Share operator*(Share me, const Share& other) {
    me *= other;
    return me;
  }

  share* get() const { return sh.get(); }
  Circuit* get_circuit() const { return circ; };
  uint32_t get_bitlen() const { return sh.get()->get_bitlength(); }
  uint32_t get_nvals() const { return sh.get()->get_nvals(); }
  e_sharing get_type() const { return sh.get()->get_share_type(); }
  virtual void reset() { circ = nullptr; sh.reset(); }
  bool is_null() const { return !(bool)sh; }

  explicit operator bool() const noexcept { return (bool)sh; };

  Share repeat(uint32_t n) {
    return Share{circ, circ->PutRepeaterGate(n, sh.get())};
  }

  /**
   * Splits a single-wired simd share into a share with nvals wires with
   * nvals=1
   * TODO directly build nvals shares instead?
   */
  Share split() {
    return Share{circ, circ->PutSplitterGate(sh.get())};
  }

  protected:
  Circuit* circ; // we don't handle memory of Circuit object
  std::shared_ptr<share> sh;
};

class BoolShare: public Share {
  public:
  BoolShare(): Share{}, bcirc{} {};
  BoolShare(BooleanCircuit* bc, share* s) :
    Share(static_cast<Circuit*>(bc), s), bcirc{bc} {};
  BoolShare(BooleanCircuit* bc, const vector<uint32_t>& gates) :
    BoolShare{bc, new boolshare(gates, static_cast<Circuit*>(bc))} {};
  // Consume Share move contructor
  BoolShare(Share&& s) :
    Share{s}, bcirc{dynamic_cast<BooleanCircuit*>(circ)} {};
  ~BoolShare() = default;

  void reset() { bcirc = nullptr; Share::reset(); }

  BooleanCircuit* get_circuit() const { return bcirc; };

  /*
   * Constructor to create new INGate from plain-text value
   */
  template<typename T>
    // require T of type uintXX_t or uintXX_t*
  BoolShare(BooleanCircuit* bc, T value, uint32_t bitlen, e_role role) :
    Share{static_cast<Circuit*>(bc), value, bitlen, role}, bcirc{bc} {}

  /*
   * Constructor to create new SIMDINGate from plain-text value
   */
  template<typename T>
    // require T of type uintXX_t or uintXX_t*
  BoolShare(BooleanCircuit* bc, T value, uint32_t bitlen, e_role role, uint32_t nvals) :
    Share{static_cast<Circuit*>(bc), value, bitlen, role, nvals}, bcirc{bc} {}

  /*
   * DummyInGate
   */
  BoolShare(BooleanCircuit* bc, uint32_t bitlen) :
    Share{static_cast<Circuit*>(bc), bitlen}, bcirc{bc} {}

  /*
   * DummySIMDINGate
   */
  BoolShare(BooleanCircuit* bc, uint32_t bitlen, uint32_t nvals) :
    Share{static_cast<Circuit*>(bc), bitlen, nvals}, bcirc{bc} {}

  BoolShare& operator&=(const BoolShare& other) {
    assert(bcirc == other.bcirc);
    sh = share_p{bcirc->PutANDGate(sh.get(), other.sh.get())};
    return *this;
  }

  BoolShare& operator^=(const BoolShare& other){
    assert(bcirc == other.bcirc);
    sh = share_p{bcirc->PutXORGate(sh.get(), other.sh.get())};
    return *this;
  }

  BoolShare& operator|=(const BoolShare& other) {
    assert(bcirc == other.bcirc);
    sh = share_p{bcirc->PutORGate(sh.get(), other.sh.get())};
    return *this;
  }

  friend BoolShare operator&(BoolShare me, const BoolShare& other) {
    me &= other;
    return me;
  }

  friend BoolShare operator^(BoolShare me, const BoolShare& other) {
    me ^= other;
    return me;
  }

  friend BoolShare operator|(BoolShare me, const BoolShare& other) {
    me |= other;
    return me;
  }

  friend BoolShare operator==(BoolShare me, const BoolShare& other) {
    assert(me.bcirc == other.bcirc);
    me.sh = share_p{me.bcirc->PutEQGate(me.sh.get(), other.sh.get())};
    return me;
  }

  friend BoolShare operator>(BoolShare me, const BoolShare& other) {
    assert(me.bcirc == other.bcirc);
    me.sh = share_p{me.bcirc->PutGTGate(me.sh.get(), other.sh.get())};
    return me;
  }

  friend BoolShare operator<(BoolShare me, const BoolShare& other) {
    assert(me.bcirc == other.bcirc);
    me.sh = share_p{me.bcirc->PutGTGate(other.sh.get(), me.sh.get())};
    return me;
  }

  friend BoolShare operator<<(BoolShare me, uint32_t shift) {
    me.sh = share_p{me.bcirc->PutLeftShifterGate(me.get(), shift)};
    return me;
  }

  BoolShare mux(const BoolShare& sh_true, const BoolShare& sh_false) {
    return BoolShare{bcirc, bcirc->PutMUXGate(sh_true.sh.get(), sh_false.sh.get(), sh.get())};
  }

  // Unary functions
  BoolShare operator~() const {
    return BoolShare{bcirc, bcirc->PutINVGate(sh.get())};
  }

  /**
   * Returns this share zeropadded to reach given bitlen
   */
  BoolShare zeropad(uint32_t bitlen) const;

  friend BoolShare hammingweight(const BoolShare& s) {
    return BoolShare{s.bcirc, s.bcirc->PutHammingWeightGate(s.get())};
  }

  // Binary functions
  friend BoolShare apply_file_binary(const BoolShare& a, const BoolShare& b,
      uint32_t a_bits, uint32_t b_bits, const string& fn);

  /**
   * Spits a simd share into ceil(nvals/new_nvals) shares
   */
  vector<BoolShare> split(uint32_t new_nval);


  protected:
  BooleanCircuit* bcirc;
};

class ArithShare: public Share {
  public:
  ArithShare(): Share{}, acirc{} {};
  ArithShare(ArithmeticCircuit* ac, share* s) :
    Share(static_cast<Circuit*>(ac), s), acirc{ac} {};
  // Consume Share move contructor
  ArithShare(Share&& s) :
    Share{std::move(s)}, acirc{dynamic_cast<ArithmeticCircuit*>(circ)} {};
  ~ArithShare() = default;

  void reset() { acirc = nullptr; Share::reset(); }

  /*
   * Constructor to create new INGate from plain-text value
   */
  template<typename T>
    // require T of type uintXX_t or uintXX_t*
  ArithShare(ArithmeticCircuit* ac, T value, uint32_t bitlen, e_role role) :
    Share{static_cast<Circuit*>(ac), value, bitlen, role}, acirc{ac} {}

  /*
   * Constructor to create new SIMDINGate from plain-text value
   */
  template<typename T>
    // require T of type uintXX_t or uintXX_t*
  ArithShare(ArithmeticCircuit* ac, T value, uint32_t bitlen, e_role role, uint32_t nvals) :
    Share{static_cast<Circuit*>(ac), value, bitlen, role, nvals}, acirc{ac} {}

  /*
   * DummyInGate
   */
  ArithShare(ArithmeticCircuit* ac, uint32_t bitlen) :
    Share{static_cast<Circuit*>(ac), bitlen}, acirc{ac} {}

  /*
   * DummySIMDINGate
   */
  ArithShare(ArithmeticCircuit* ac, uint32_t bitlen, uint32_t nvals) :
    Share{static_cast<Circuit*>(ac), bitlen, nvals}, acirc{ac} {}

  protected:
  ArithmeticCircuit* acirc;
};

class OutShare: private Share {
  public:
  OutShare(Circuit* circ, share* sh) : Share{circ, sh} {}

  template<typename T>
    // require T of type uint*_t
  T get_clear_value() {
    return sh->get_clear_value<T>();
  }

  void write_clear_value_vec(uint32_t** vec, uint32_t* bitlen, uint32_t* nvals) {
    return sh->get_clear_value_vec(vec, bitlen, nvals);
  }

  vector<uint32_t> get_clear_value_vec();
};

/******************** Factories ********************/
/*
 * OutShare factory
 * Clear value will be learned by dst
 */
OutShare out(const Share& share, e_role dst);

/*
 * OutShare factory
 * Clear value of OutShare will actually hold a share of the underlying value
 * for each party, for assemblage at a later stage.
 */
OutShare out_shared(const Share& share);

/*
 * Debugging PrintValueGate
 */
OutShare print_share(const Share& share, const string& msg);

Share constant(Circuit* c, UGATE_T val, uint32_t bitlen);

BoolShare constant(BooleanCircuit* c, UGATE_T val, uint32_t bitlen);

Share constant_simd(Circuit* c, UGATE_T val, uint32_t bitlen, uint32_t nvals);

BoolShare constant_simd(BooleanCircuit* c, UGATE_T val, uint32_t bitlen, uint32_t nvals);

BoolShare constant_simd(ArithmeticCircuit* c, UGATE_T val, uint32_t bitlen, uint32_t nvals);

/******************** Conversions ********************/

BoolShare a2y(BooleanCircuit* ycirc, const ArithShare& s);

ArithShare y2a(ArithmeticCircuit* acirc, BooleanCircuit* bcirc, const BoolShare& s);

BoolShare a2b(BooleanCircuit* bcirc, BooleanCircuit* ycirc, const ArithShare& s);

ArithShare b2a(ArithmeticCircuit* acirc, const BoolShare& s);

BoolShare y2b(BooleanCircuit* bcirc, const BoolShare& s);

BoolShare b2y(BooleanCircuit* ycirc, const BoolShare& s);

/******************** SIMD stuff ********************/

/**
 * Vertically Combines the given shares to a new share having the same number of
 * wires=bitlen and nvals the sum of the individual nvals
 */
BoolShare vcombine(const vector<BoolShare>& shares);

} // namespace sel

#endif /* end of include guard: SEL_ABY_SHARE_H */
