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
#include <map>
#include <algorithm>
#include <iterator>
#include <functional>
#include <memory>
#include <stdexcept>
#include <cctype>
#include <locale>
#include <sstream>
#include "fmt/format.h"
#include "epilink_input.h"

using Bitmask = std::vector<uint8_t>;

namespace sel {

constexpr size_t bitbytes(size_t b) { return (b + 7)/8; }

/**
 * Ceiling of integer log2
 */
int ceil_log2(unsigned long long x);
int ceil_log2_min1(unsigned long long x);

/**
 * Concatenates the vectors to a single vector, i.e., it flattenes the given
 * vector of vectors.
 */
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

/**
 * Repeats the vectors n times.
 */
template <typename T>
std::vector<T> repeat_vec(const std::vector<T>& v, const size_t n) {
  std::vector<T> c;
  c.reserve(v.size() * n);
  for (size_t i = 0; i != n; ++i) {
    c.insert(c.end(), v.begin(), v.end());
  }
  return c;
}

/**
 * Generates a bitmaks of given size (rounded up to next multiple of 8) with
 * given bit set at all positions.
 */
std::vector<uint8_t> repeat_bit(const bool bit, const size_t n);

std::vector<uint8_t> vector_bool_to_bitmask(const std::vector<bool>&);

/**
 * Hammingweight/popcount of bitmask
 */
size_t hw(const Bitmask& bm);

/**
 * Performs bitwise AND (&) on both bitmasks' bits
 */
Bitmask bm_and(const Bitmask& left, const Bitmask& right);

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

template <typename InType, typename Transformer,
  typename OutType = decltype(std::declval<Transformer>()(std::declval<InType>()))>
std::vector<OutType> transform_vec(const std::vector<InType>& vec,
    Transformer op) {
  std::vector<OutType> out;
  out.reserve(vec.size());
  std::transform(vec.cbegin(), vec.cend(), std::back_inserter(out), op);
  return out;
}

/**
 * Transforms the values of the given map with the given transformation function
 * and returns the transformed map with the same keys.
 * https://stackoverflow.com/questions/50881383/stdmap-transformer-template
 */
template <class Key, class FromValue, class Transformer,
  class ToValue = decltype(std::declval<Transformer>()(std::declval<FromValue>()))>
std::map<Key, ToValue> transform_map(const std::map<Key, FromValue>& _map,
    Transformer _tr) {
  std::map<Key, ToValue> res;
  std::for_each(_map.cbegin(), _map.cend(),
      [&res, &_tr](const std::pair<const Key, FromValue>& kv) {
        res[kv.first] = _tr(kv.second);
      });
  return res;
}

/**
 * Transforms the values of the given map with the given transformation function
 * and returns the transformed key-value pairs as vector
 * https://stackoverflow.com/questions/50881383/stdmap-transformer-template
 */
template <class Key, class FromValue, class Transformer,
  class ToValue = decltype(std::declval<Transformer>()
      (std::declval<std::pair<const Key, FromValue>>()))>
std::vector<ToValue> transform_map_vec(const std::map<Key, FromValue>& _map,
    Transformer _tr) {
  std::vector<ToValue> res;
  res.reserve(_map.size());
  std::for_each(_map.cbegin(), _map.cend(),
      [&res, &_tr](const std::pair<const Key, FromValue>& kv) {
        res.emplace_back(_tr(kv));
      });
  return res;
}

/**
 * Return maximum element (by value) from given vector
 */
template <class Value>
Value max_element(const std::vector<Value>& _vec) {
  return *(std::max_element(_vec.begin(),_vec.end()));
}

/**
 * Return maximum element (by value) from given map on which the transformer is
 * applied
 */
template <class Key, class FromValue, class Transformer,
  class ToValue = decltype(std::declval<Transformer>()
      (std::declval<std::pair<const Key, FromValue>>()))>
ToValue max_element(const std::map<Key, FromValue>& _map, Transformer _tr) {
  return sel::max_element(transform_map_vec(_map,_tr));
}

/**
 * Print vectors to cout
 */
template<typename T>
std::ostream& operator<< (std::ostream& out, const std::vector<T>& v) {
    std::ios_base::fmtflags outf( out.flags() ); // save cout flags
    out << "[" << std::hex;
    size_t last = v.size() - 1;
    for(size_t i = 0; i < v.size(); ++i) {
        out << v[i];
        if (i != last)
            out << ", ";
    }
    out << "]";
    out.flags(outf); // restore old flags
    return out;
}

// specialization for uint8_t / unsigned char
std::ostream& operator<< (std::ostream& out, const std::vector<uint8_t>& v);

// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s) {
    ltrim(s);
    return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s) {
    rtrim(s);
    return s;
}

// trim from both ends (copying)
static inline std::string trim_copy(std::string s) {
    trim(s);
    return s;
}
template<typename Out>
void split(const std::string &s, char delim, Out result) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}
std::vector<std::string> split(const std::string &s, char delim);
std::string generate_id();

std::istream& safeGetline(std::istream&, std::string&);
std::vector<std::string> get_headers(std::istream& is,const std::string& header);

// Forward Declarations
class LocalConfiguration;
struct AlgorithmConfig;
EpilinkConfig get_epilink_config(std::shared_ptr<const LocalConfiguration>, std::shared_ptr<const AlgorithmConfig>);
} // namespace sel

// Custom fmt formatters for our types
namespace fmt {

/**
 * Container printer (vector, set, ...)
 * inspired by https://github.com/louisdx/cxx-prettyprint
 */
template <typename T, template<typename...> class Container>
struct formatter<Container<T>> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const Container<T> v, FormatContext &ctx) {
    auto c = format_to(ctx.begin(), "[");

    auto it = std::cbegin(v);
    auto the_end = std::cend(v);

    for (;;) {
      c = format_to(c, "{}", *it);
      if (++it == the_end) break;
      c = format_to(c, ", ");
    }
    return format_to(c,"]");
  }
};

template<> struct formatter<Bitmask> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const Bitmask v, FormatContext &ctx) {
    auto c = ctx.begin();
    for (auto e = v.cbegin(); e != v.cend(); ++e) {
      c = format_to(c, "{:x}", *e);
      // separate each 2 bytes by whitespace
      if ((e-v.cbegin())%2) {
        c = format_to(c, " ");
      }
    }
    return c;
  }
};

} // namespace fmt

#endif /* end of include guard: SEL_UTIL_H */
