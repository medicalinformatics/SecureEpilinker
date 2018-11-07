/**
 \file    sel/aby/quotient_folder.h
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
 \brief Fold Share Quotients with min or max
        * may choose to apply more complex tie logix
        * may select same index in other (vector of) target share(s)
*/

#ifndef SEL_ABY_QUOTIENT_FOLDER_H
#define SEL_ABY_QUOTIENT_FOLDER_H
#pragma once

#include "gadgets.h"

namespace sel {

template <class ShareT>
class QuotientFolder {
  static constexpr bool do_conversion = std::is_same_v<ShareT, ArithShare>;
public:
  enum class FoldOp { MIN, MIN_TIE, MAX, MAX_TIE };

  QuotientFolder(Quotient<ShareT>&& selector, std::vector<BoolShare>&& targets,
      FoldOp _fold_op = FoldOp::MAX_TIE)
    : base{std::forward<Quotient<ShareT>>(selector),
           std::forward<std::vector<BoolShare>>(targets)},
      fold_op{_fold_op}
  {
    if constexpr (!do_conversion) {
      static const T2BConverter<BoolShare> bool_identity = [](auto x){return x;};
      to_bool = &bool_identity;
    }
  }

  void set_fold_operation(FoldOp _fold_op) {
    fold_op = _fold_op;
  }

  /**
   * Set converters and denominator bits
   * We only convert back and forth if we use arithmetic shares.
   * In this case, it also makes sense to tell the bitsize of denominators, so
   * that arithmetic shares that are converted back to boolean space can be
   * truncated.
   */
  std::enable_if_t<do_conversion> set_converters_and_den_bits(
    T2BConverter<ShareT> const* _to_bool, B2AConverter const* _to_arith,
    const size_t _den_bits = 0) {
      to_bool = _to_bool;
      to_arith = _to_arith;
      den_bits = _den_bits;
  }

  class Leaf {
  public:
    Leaf() = default;
    Leaf(Quotient<ShareT>&& _selector, std::vector<BoolShare>&& _targets)
    : selector{std::forward<Quotient<ShareT>>(_selector)},
      targets{std::forward<std::vector<BoolShare>>(_targets)}
    {
      size_t nvals0 = selector.num.get_nvals();
      assert (selector.den.get_nvals() == nvals0);
      for (const auto& t : targets) assert(t.get_nvals() == nvals0);
    }

    bool empty() const {
      bool res = selector.den.is_null();
      assert (selector.num.is_null() == res);
      for (auto& t : targets) assert (t.is_null() == res);
      return res;
    }

    size_t size() const {
      if (empty()) return 0;
      return selector.num.get_nvals();
    }

    void reset() {
      selector.num.reset();
      selector.den.reset();
      for (auto& t : targets) t.reset();
    }

    auto const& get_selector() { return selector; }
    auto const& get_targets() { return targets; }

  protected:
    friend class QuotientFolder<ShareT>;
    Quotient<ShareT> selector;
    std::vector<BoolShare> targets;

    bool odd_size() const { return size() % 2; }

    Leaf& append(const Leaf& other) {
      selector.num = vcombine<ShareT>({selector.num, other.selector.num});
      selector.den = vcombine<ShareT>({selector.den, other.selector.den});
      assert (targets.size() == other.targets.size());
      for(size_t i=0; i!=targets.size(); ++i) {
        targets[i] = vcombine<BoolShare>({targets[i], other.targets[i]});
      }
      return *this;
    }

    /**
     * Takes the i'th slice in all split result vectors
     */
    static Leaf slice_vec(const std::vector<ShareT>& nums,
        const std::vector<ShareT>& dens,
        const std::vector<std::vector<BoolShare>>& targetss, size_t i) {
      auto targets = transform_vec(targetss, [&i](const std::vector<BoolShare>& ts) { return ts[i]; });
      return {{std::move(nums[i]), std::move(dens[i])}, std::move(targets)};
    }
  };

  Leaf fold() {
    auto op_select = make_selector();

    while (base.size() > 1) {
      split_half();
      fold_once(other, op_select);
      if (base.odd_size() && have_remainder()) append_remainder();
    }
    if (have_remainder()) fold_once(remainder, op_select);
    return base;
  }

private:
  Leaf base, other, remainder;
  FoldOp fold_op;
  T2BConverter<ShareT> const* to_bool = nullptr;
  B2AConverter const* to_arith = nullptr;
  size_t den_bits = 0;

  QuotientSelector<ShareT> make_selector() {
    switch (fold_op) {
      case FoldOp::MIN:
        return make_min_selector(*to_bool);
      case FoldOp::MAX:
        return make_max_selector(*to_bool);
      case FoldOp::MIN_TIE:
        return make_min_tie_selector(*to_bool, den_bits);
      default: // MAX_TIE is default
        return make_max_tie_selector(*to_bool, den_bits);
    }
  }

  bool have_remainder() const { return remainder.size() > 0; }
  void append_remainder() {
#ifdef DEBUG_SEL_GADGETS
    std::cout << ">> appending remainder\n";
#endif
    base.append(remainder);
    remainder.reset();
  }

  void split_half() {
    size_t half_size = base.size()/2;
#ifdef DEBUG_SEL_GADGETS
    std::cout << "> Splitting " << base.size() << " to " << half_size
      << " % " << base.size()%2 << '\n';
#endif
    auto splits_num = base.selector.num.split(half_size);
    auto splits_den = base.selector.den.split(half_size);
    auto split_targets = transform_vec(base.targets,
        [&half_size](const BoolShare& target) { return target.split(half_size); });

    base = Leaf::slice_vec(splits_num, splits_den, split_targets, 0);
    other = Leaf::slice_vec(splits_num, splits_den, split_targets, 1);
    assert (base.size() == half_size);
    assert (other.size() == half_size);
    // check remainder
    if (splits_num.size() > 2) {
#ifdef DEBUG_SEL_GADGETS
      std::cout << ">> setting split remainder\n";
#endif
      assert (splits_num.size() == 3);
      assert (splits_den.size() == 3);
      remainder = Leaf::slice_vec(splits_num, splits_den, split_targets, 2);
      assert (remainder.size() == 1);
    }
  }

  void fold_once(const Leaf& with, const QuotientSelector<ShareT>& op_select) {
    assert(base.size() == with.size());
    auto selection = op_select(base.selector, with.selector);
    if constexpr (do_conversion) {
      ArithShare arith_sel = (*to_arith)(selection);
      ArithmeticCircuit* ac = base.selector.num.get_circuit();
      ArithShare one = constant_simd(ac, 1u, ac->GetShareBitLen(), base.size());
      ArithShare not_sel = one - arith_sel;
      base.selector.num = arith_sel * base.selector.num + not_sel * with.selector.num;
      base.selector.den = arith_sel * base.selector.den + not_sel * with.selector.den;
    } else {
      base.selector.num = selection.mux(base.selector.num, with.selector.num);
      base.selector.den = selection.mux(base.selector.den, with.selector.den);
    }
    for(size_t i=0; i!=base.targets.size(); ++i)
      base.targets[i] = selection.mux(base.targets[i], with.targets[i]);
  }
};


} /* end of namespace: sel */

#endif /* end of include guard: SEL_ABY_QUOTIENT_FOLDER_H */
