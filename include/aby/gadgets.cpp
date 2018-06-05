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
#include "circuit_defs.h"
#include "../util.h"

using namespace std;

namespace sel {

/**
 * Accumulates all objects of vector using binary operation op in a balanced
 * binary tree structure.
 */
template <class ShareT>
ShareT binary_accumulate(vector<ShareT> vals,
    const function<ShareT (const ShareT&, const ShareT&)>& op) {
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

template Share binary_accumulate(vector<Share> vals,
    const BinaryOp_Share& op);

template ArithShare binary_accumulate(vector<ArithShare> vals,
    const BinaryOp_ArithShare& op);

template BoolShare binary_accumulate(vector<BoolShare> vals,
    const BinaryOp_BoolShare& op);

/**
 * Accumulates all objects of vector using binary operation op serially by
 * left-fold
 */
template <class ShareT>
ShareT lfold_accumulate(const vector<ShareT>& vals,
    const function<ShareT (const ShareT&, const ShareT&)>& op) {
  ShareT acc(vals.at(0));
  for (size_t i{1}; i != vals.size(); ++i) {
    acc = op(acc, vals[i]);
  }

  return acc;
}

/**
 * Chooses best accumulation according to the sharing type
 */
template <class ShareT> //,
  //typename std::enable_if<std::is_base_of<Share, ShareT>::value>::type>
ShareT best_accumulate(const vector<ShareT>& vals,
    const function<ShareT (const ShareT&, const ShareT&)>& op) {
  return (vals.at(0).get_type() == S_YAO) ?
    lfold_accumulate(vals, op) : binary_accumulate(vals, op);
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

BoolShare sum(const vector<BoolShare>& shares) {
  function<BoolShare(const BoolShare&, const BoolShare&)> op =
      [](const BoolShare& a, const BoolShare& b) {return a + b;};
  return best_accumulate(shares, op);
}

ArithShare sum(const vector<ArithShare>& shares) {
  function<ArithShare(const ArithShare&, const ArithShare&)> op =
      [](const ArithShare& a, const ArithShare& b) {return a + b;};
  return lfold_accumulate(shares, op);
}

/**
 * Takes a simd share and recursively splits it in two and applies op.
 * This way, all nval shares are accumulated with depth log(nvals) and
 * log(nvals) simd operations as if simd_share was completely split into nval
 * shares and then binary_accumulate()'d.
 */
BoolShare split_accumulate(BoolShare simd_share, const BinaryOp_BoolShare& op) {
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
      simd_share = vcombine({simd_share, move(stack_share)});
      assert(stack_share.is_null());
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

} // namespace sel
