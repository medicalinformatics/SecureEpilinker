
/**
 \file    sel/math.h
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
 \brief Math tools
*/

#ifndef SEL_MATH_H
#define SEL_MATH_H
#pragma once

namespace sel {

template<typename T>
T factorial(T x) {
  T y{1};
  for(T i = 1; i != x; ++i)
    y *= i;
  return y;
}

} // namespace sel
#endif /* end of include guard: SEL_MATH_H */
