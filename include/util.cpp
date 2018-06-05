/**
 \file    sel/util.h
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
 \brief general utilities
*/

#include "util.h"

using namespace std;

namespace sel {

std::vector<uint8_t> vector_bool_to_bitmask(const std::vector<bool>& vb) {
  vector<uint8_t> v;
  size_t size_bytes{bitbytes(vb.size())};
  v.reserve(size_bytes);
  uint8_t x{0};
  size_t i{0}; // we reuse i for remaining bits
  for (; i != size_bytes; ++i, x = 0) {
    // in last iteration, only set remaining bits
    int bitsleft = (i == size_bytes -1) ? (vb.size()-1)%8+1 : 8;
    for (int j{0}; j != bitsleft; ++j)
      if (vb[8*i+j]) x |= (1 << j);

    v.emplace_back(x);
  }

  return v;
}

} // namespace sel
