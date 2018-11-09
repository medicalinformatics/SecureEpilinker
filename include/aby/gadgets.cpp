/**
 \file    sel/aby/gadgets.cpp
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

#include <memory>
#include <type_traits>
#include "gadgets.h"
#include "../util.h"

#ifdef DEBUG_SEL_GADGETS
#include <fmt/format.h>
using fmt::format;
using fmt::print;
#endif

using namespace std;

namespace sel {

BoolShare bool_identity(const BoolShare& x) { return x; }

/**
 * Accumulates all objects of vector using binary operation op in a balanced
 * binary tree structure.
 */
template <class T>
T binary_accumulate(vector<T> vals,
    const function<T (const T&, const T&)>& op) {
  for(size_t j, n{vals.size()}; n > 1; n = j) {
    j = 0;
    for(size_t i{0}; i < n; ++j) {
      if (i + 1 >= n) { // only i+1 == n possible
        vals[j] = vals[i];
        ++i;
      } else {
        vals[j] = op(vals[i], vals[i + 1]);
        i += 2;
      }
    }
  }

  return vals.at(0);
}

template ArithShare binary_accumulate(vector<ArithShare> vals,
    const BinaryOp<ArithShare>& op);

template BoolShare binary_accumulate(vector<BoolShare> vals,
    const BinaryOp<BoolShare>& op);

template <class ShareT>
void print_share(const Quotient<ShareT>& q, const string& msg) {
  print_share(q.num, msg + "(num)");
  print_share(q.den, msg + "(den)");
}
template void print_share(const BoolQuotient& q, const string& msg);
template void print_share(const ArithQuotient& q, const string& msg);

/**
 * Accumulates all objects of vector using binary operation op serially by
 * left-fold
 */
template <class ShareT>
ShareT lfold_accumulate(const vector<ShareT>& vals, const BinaryOp<ShareT>& op) {
  ShareT acc = vals.at(0);
  for (size_t i{1}; i != vals.size(); ++i) {
    acc = op(acc, vals[i]);
  }

  return acc;
}

BoolShare max(const vector<BoolShare>& shares) {
  BooleanCircuit* circ = shares.at(0).get_circuit();

  /* Don't know why, but doesn't work...
  vector<share*> s_max = transform_vec(shares,
      function<share*(const BoolShare&)>([](const BoolShare& sh)
        -> share* {return sh.get();}));
  */
  vector<share*> shs(shares.size());
  transform(shares.cbegin(), shares.cend(), shs.begin(),
      [](const BoolShare& sh) {return sh.get();});
  return BoolShare(circ, circ->PutMaxGate(shs));
}

template <class ShareT>
ShareT sum(const vector<ShareT>& shares) {
  BinaryOp<ShareT> op = [](auto a, auto b) {return a + b;};
  return binary_accumulate(shares, op);
}
template BoolShare sum(const vector<BoolShare>&);
template ArithShare sum(const vector<ArithShare>&);

/**
 * Takes a simd share and recursively splits it in two and applies op.
 * This way, all nval shares are accumulated with depth log(nvals) and
 * log(nvals) simd operations as if simd_share was completely split into nval
 * shares and then binary_accumulate()'d.
 */
BoolShare split_accumulate(BoolShare simd_share, const BinaryOp<BoolShare>& op) {
#ifdef DEBUG_SEL_GADGETS
  cout << "==== split-accumulating share of nvals: " << simd_share.get_nvals() << " ====\n";
#endif
  //size_t stack_nval{0};
  //vector<BoolShare> stack;
  // TODO make algo more general to align to certain
  // number of blocks instead of 1, so need to collect remainder shares on a
  // stack
  BoolShare stack_share;
  assert (!(bool)stack_share);
  for(size_t cnvals{simd_share.get_nvals()/2}, rem{simd_share.get_nvals()%2};
      simd_share.get_nvals() > 1; rem = cnvals%2, cnvals /=2) {
#ifdef DEBUG_SEL_GADGETS
    cout << "cnvals: " << cnvals << " rem: " << rem << "\n";
#endif
    /* accumulate with stack if it has grown over cnvals
    while (stack_nval >= cnvals) {
      BoolShare stack_share = vcombine(stack);
      assert(stack_share.get_nvals() == stack_nval);
      //reset stack
      stack.clear();
      stack_nval = 0;
    } */ // TODO only for more general algo
    if (rem && stack_share) {
#ifdef DEBUG_SEL_GADGETS
      cout << "remainder and stack: combining stack and cnvals++\n";
#endif
      simd_share = vcombine<BoolShare>({simd_share, stack_share});
      stack_share.reset();
      cnvals++;
      rem = 0;
    }

    vector<BoolShare> splits = simd_share.split(cnvals);

    // push to stack if remainder
    if (splits.size() > 2) {
#ifdef DEBUG_SEL_GADGETS
      cout << "storing remainder stack\n";
#endif
      assert (splits.size() == 3);
      assert (rem == 1);
      stack_share = move(splits.back());
      assert (stack_share.get_nvals() == 1);
    }
    assert (splits[0].get_nvals() == cnvals);
    assert (splits[1].get_nvals() == cnvals);
    simd_share = op(splits[0], splits[1]);
  }
  // finally accumulate with stack
  if (!stack_share.is_null()) {
#ifdef DEBUG_SEL_GADGETS
    cout << "stack not empty, final acc" << endl;
#endif
    simd_share = op(simd_share, stack_share);
  }
  return simd_share;
}

void split_select_target(BoolShare& selector, BoolShare& target,
    const BinaryOp<BoolShare>& op_select) {
  assert (selector.get_nvals() == target.get_nvals());
#ifdef DEBUG_SEL_GADGETS
  cout << "==== split-select-target share of nvals: " << selector.get_nvals() << " ====\n";
#endif
  BoolShare stack_selector, stack_target;
  assert (stack_selector.is_null());
  for(size_t cnvals{selector.get_nvals()/2}, rem{selector.get_nvals()%2};
      selector.get_nvals() > 1; rem = cnvals%2, cnvals /=2) {
#ifdef DEBUG_SEL_GADGETS
    cout << "cnvals: " << cnvals << " rem: " << rem << "\n";
#endif
    if (rem && stack_selector) {
#ifdef DEBUG_SEL_GADGETS
      cout << "remainder and stack: combining stack and cnvals++\n";
#endif
      selector = vcombine<BoolShare>({selector, stack_selector});
      target = vcombine<BoolShare>({target, stack_target});
      stack_selector.reset(); // need to reset, move doesn't work :(
      stack_target.reset(); // need to reset
      assert (stack_selector.is_null());
      cnvals++;
      rem = 0;
    }

    vector<BoolShare> splits = selector.split(cnvals);
    vector<BoolShare> tsplits = target.split(cnvals);

    // push to stack if remainder
    if (splits.size() > 2) {
#ifdef DEBUG_SEL_GADGETS
      cout << "storing remainder stack\n";
#endif
      assert (splits.size() == 3 && tsplits.size() == 3);
      assert (rem == 1);
      stack_selector = move(splits.back());
      stack_target = move(tsplits.back());
      assert (stack_selector.get_nvals() == 1);
    }
    assert (splits[0].get_nvals() == cnvals);
    assert (splits[1].get_nvals() == cnvals);
    BoolShare cmp = op_select(splits[0], splits[1]);
    selector = cmp.mux(splits[0], splits[1]);
    target = cmp.mux(tsplits[0], tsplits[1]);
  }
  // finally accumulate with stack
  if (!stack_selector.is_null()) {
#ifdef DEBUG_SEL_GADGETS
    cout << "stack not empty, final acc" << endl;
#endif
    BoolShare cmp = op_select(selector, stack_selector);
    selector = cmp.mux(selector, stack_selector);
    target = cmp.mux(target, stack_target);
  }
}

template <class ShareT>
QuotientSelector<ShareT> make_tie_selector(const T2BConverter<ShareT>& to_bool,
    const BinaryOp<BoolShare>& op_select, const size_t den_bits = 0) {
  return [&to_bool, op_select, den_bits] (auto a, auto b) {
      BoolShare ax, bx, a_den, b_den;
      if constexpr(is_same_v<ShareT, ArithShare>) {
        ax = to_bool(a.num * b.den);
        bx = to_bool(b.num * a.den);
        a_den = to_bool(a.den);
        b_den = to_bool(b.den);
      } else {
        ax = a.num * b.den;
        bx = b.num * a.den;
        a_den = a.den;
        b_den = b.den;
      }

      auto quotients_equal = ax == bx;
      auto quotient_select = op_select(ax, bx);

      if (den_bits) {
        a_den.set_bitlength(den_bits);
        b_den.set_bitlength(den_bits);
      }
      auto scale_select = op_select(a_den, b_den);

      // If quotients are equal, select by scale.
      auto selection = quotient_select | (quotients_equal & scale_select);

#ifdef DEBUG_SEL_GADGETS
      static unsigned i = 0;
      const string i_str = '[' + to_string(i++) + "] ";
      print_share(a, i_str+"selector a");
      print_share(b, i_str+"selector b");
      print_share(a_den, i_str+"a_den");
      print_share(b_den, i_str+"b_den");
      print_share(quotients_equal, i_str+"quotients_equal");
      print_share(quotient_select, i_str+"quotient_select");
      print_share(scale_select, i_str+"scale_select");
      print_share(selection, i_str+"selection");
#endif

      return selection;
  };
}

template <class ShareT>
QuotientSelector<ShareT> make_max_tie_selector(const T2BConverter<ShareT>& to_bool,
    const size_t den_bits) {
  BinaryOp<BoolShare> op_select = [](auto a, auto b) { return a > b; };
  return make_tie_selector(to_bool, op_select, den_bits);
}

template
QuotientSelector<BoolShare> make_max_tie_selector(const T2BConverter<BoolShare>&, const size_t);
template
QuotientSelector<ArithShare> make_max_tie_selector(const T2BConverter<ArithShare>&, const size_t);

template <class ShareT>
QuotientSelector<ShareT> make_min_tie_selector(const T2BConverter<ShareT>& to_bool,
    const size_t den_bits) {
  BinaryOp<BoolShare> op_select = [](auto a, auto b) { return a < b; };
  return make_tie_selector(to_bool, op_select, den_bits);
}

template
QuotientSelector<BoolShare> make_min_tie_selector(const T2BConverter<BoolShare>&, const size_t);
template
QuotientSelector<ArithShare> make_min_tie_selector(const T2BConverter<ArithShare>&, const size_t);

template <class ShareT>
QuotientSelector<ShareT> make_max_selector(const T2BConverter<ShareT>& to_bool) {
  return [&to_bool] (auto a, auto b) {
      ShareT ax = a.num * b.den;
      ShareT bx = b.num * a.den;
      return to_bool(ax) > to_bool(bx);
  };
}
template
QuotientSelector<BoolShare> make_max_selector(const T2BConverter<BoolShare>&);
template
QuotientSelector<ArithShare> make_max_selector(const T2BConverter<ArithShare>&);

template <class ShareT>
QuotientSelector<ShareT> make_min_selector(const T2BConverter<ShareT>& to_bool) {
  return [&to_bool] (auto a, auto b) {
      ShareT ax = a.num * b.den;
      ShareT bx = b.num * a.den;
      return to_bool(ax) < to_bool(bx);
  };
}
template
QuotientSelector<BoolShare> make_min_selector(const T2BConverter<BoolShare>&);
template
QuotientSelector<ArithShare> make_min_selector(const T2BConverter<ArithShare>&);

// TODO#13 If we can mux with ArithShares, use it here.
ArithQuotient select_quotient(const ArithQuotient& a, const ArithQuotient& b,
    const QuotientSelector<ArithShare>& op_select, const B2AConverter& to_arith) {
  /* Old implementation uses bool muxing
  BoolQuotient q = max(a, b, to_bool);
  return {to_arith(q.num), to_arith(q.den)};
  */
  uint32_t nvals  = a.num.get_nvals();
  assert(a.den.get_nvals() == nvals);
  assert(b.num.get_nvals() == nvals);
  assert(b.den.get_nvals() == nvals);
  const auto cmp = op_select(a, b);
  ArithShare acmp = to_arith(cmp);
  ArithmeticCircuit* acirc = acmp.get_circuit();
  ArithShare one = constant_simd(acirc, 1u, acirc->GetShareBitLen(), nvals);
  ArithShare notcmp = one - acmp;
#ifdef DEBUG_SEL_GADGETS
  print_share(cmp, "select_quotient cmp");
  print_share(acmp, "select_quotient acmp");
  print_share(notcmp, "select_quotient notcmp");
#endif

  ArithShare num = acmp * a.num + notcmp * b.num;
  ArithShare den = acmp * a.den + notcmp * b.den;
  return {move(num), move(den)};
}

BoolQuotient select_quotient(const BoolQuotient& a, const BoolQuotient& b,
    const QuotientSelector<BoolShare>& op_select) {
  const auto selection = op_select(a, b);
  return {selection.mux(a.num, b.num), selection.mux(a.den, b.den)};
}

ArithQuotient max_tie(const vector<ArithQuotient>& qs,
    const A2BConverter& to_bool, const B2AConverter& to_arith,
    const size_t den_bits) {
  auto op_select = make_max_tie_selector(to_bool, den_bits);
  return binary_accumulate(qs, (BinaryOp<ArithQuotient>)
      [&op_select, &to_arith](auto a, auto b) {
      return select_quotient(a, b, op_select, to_arith);
      });
}

BoolQuotient max_tie(const vector<BoolQuotient>& qs) {
  auto op_select = make_max_tie_selector<BoolShare>(bool_identity);
  return binary_accumulate(qs, (BinaryOp<BoolQuotient>)
      [&op_select](auto a, auto b) {
      return select_quotient(a, b, op_select);
      });
}

ArithQuotient max(const ArithQuotient& a, const ArithQuotient& b,
    const A2BConverter& to_bool, const B2AConverter& to_arith) {
  auto op_select = make_max_selector(to_bool);
  return select_quotient(a, b, op_select, to_arith);
}

ArithQuotient max(const vector<ArithQuotient>& qs,
    const A2BConverter& to_bool, const B2AConverter& to_arith) {
  return binary_accumulate(qs, (BinaryOp<ArithQuotient>)
      [&to_bool, &to_arith](auto a, auto b) {
      return max(a, b, to_bool, to_arith);
      });
}

BoolQuotient max(const ArithQuotient& a, const ArithQuotient& b,
    const A2BConverter& to_bool) {
  uint32_t nvals  = a.num.get_nvals();
  __ignore(nvals);
  assert(a.den.get_nvals() == nvals);
  assert(b.num.get_nvals() == nvals);
  assert(b.den.get_nvals() == nvals);
  ArithShare ax = a.num * b.den;
  ArithShare bx = b.num * a.den;
  // convert to bool
  BoolShare b_ax = to_bool(ax), b_bx = to_bool(bx);
  BoolShare cmp = (b_ax > b_bx);

  BoolShare num = cmp.mux(to_bool(a.num), to_bool(b.num));
  BoolShare den = cmp.mux(to_bool(a.den), to_bool(b.den));
  return {move(num), move(den)};
}

BoolShare ascending_numbers_constant(BooleanCircuit* bcirc,
    size_t nvals, size_t start) {
  // TODO Make true SIMD constants available in ABY and implement offline
  // AND with constant
  vector<BoolShare> numbers;
  numbers.reserve(nvals);
  size_t end = nvals + start;
  for (size_t i = start; i != end; ++i) {
    numbers.emplace_back(constant(bcirc, i, ceil_log2_min1(end)));
  }
  return vcombine<BoolShare>(numbers);
}

} // namespace sel
