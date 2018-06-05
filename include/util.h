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

#ifndef SEL_UTIL_H
#define SEL_UTIL_H
#pragma once

#include <vector>
#include <algorithm>
#include <iterator>
#include <functional>
#include <memory>
#include <fmt/format.h>
#include <stdexcept>

namespace sel {

constexpr size_t bitbytes(size_t b) { return (b + 7)/8; }

/* dunno why u not working */
template <typename T>
std::vector<T> concat_vec(const std::vector<std::vector<T>>& vs) {
  size_t total_size{0};
  for (auto& v : vs) total_size += v.size();

  std::vector<T> c;
  c.reserve(total_size);
  for (auto& v : vs) {
    c.insert(c.end(), v.begin(), v.end());
  }
  return c;
}

std::vector<uint8_t> vector_bool_to_bitmask(const std::vector<bool>&);

/*
std::vector<uint8_t> concat_vec(const std::vector<std::vector<uint8_t>>& vs) {
  size_t total_size{0};
  for (auto& v : vs) total_size += v.size();

  std::vector<uint8_t> c;
  c.reserve(total_size);
  for (auto& v : vs) {
    c.insert(c.end(), v.begin(), v.end());
  }
  return c;
}
*/
template <typename InType,
  template <typename U, typename alloc = std::allocator<U>>
            class InContainer,
  template <typename V, typename alloc = std::allocator<V>>
            class OutContainer = InContainer,
  typename OutType = InType>
OutContainer<OutType> transform_vec(const InContainer<InType>& vec,
   std::function<OutType(const InType&)> op) {
  OutContainer<OutType> out;
  out.reserve(vec.size());
  std::transform(vec.cbegin(), vec.cend(), std::back_inserter(out), op);
  return out;
}


template <typename T>
void check_vector_size(const std::vector<T>& r,
    const size_t& size, const std::string& name) {
  if (r.size() != size)
    throw std::invalid_argument{fmt::format(
        "check_vector_size: size mismatch: all {} vectors need to be of same size {}. Found size {}",
        name, size, r.size())};
}

template <typename T>
void check_vectors_size(const std::vector<std::vector<T>>& vec,
    const size_t& size, const std::string& name) {
  for (auto& r : vec) {
    check_vector_size(r, size, name);
  }
}

/**
 * Print vectors to cout
 */
template<typename T>
std::ostream& operator<< (std::ostream& out, const std::vector<T>& v) {
    out << "[";
    size_t last = v.size() - 1;
    for(size_t i = 0; i < v.size(); ++i) {
        out << v[i];
        if (i != last)
            out << ", ";
    }
    out << "]";
    return out;
}

} // namespace sel

#endif /* end of include guard: SEL_UTIL_H */
