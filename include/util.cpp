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
#include <sstream>
#include <iterator>
#include <algorithm>
#include <iterator>
#include <functional>
#include <iomanip>
#include <chrono>
#include <random>

using namespace std;

namespace sel {

// https://stackoverflow.com/questions/3272424/compute-fast-log-base-2-ceiling
int ceil_log2(unsigned long long x)
{
  static const unsigned long long t[6] = {
    0xFFFFFFFF00000000ull,
    0x00000000FFFF0000ull,
    0x000000000000FF00ull,
    0x00000000000000F0ull,
    0x000000000000000Cull,
    0x0000000000000002ull
  };

  int y = (((x & (x - 1)) == 0) ? 0 : 1);
  int j = 32;
  int i;

  for (i = 0; i < 6; i++) {
    int k = (((x & t[i]) == 0) ? 0 : j);
    y += k;
    x >>= k;
    j >>= 1;
  }

  return y;
}

int ceil_log2_min1(unsigned long long x) {
  return max(1, ceil_log2(x));
}

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

std::vector<uint8_t> repeat_bit(const bool bit, const size_t n) {
  return std::vector<uint8_t>(bitbytes(n), bit ? 0xffu : 0u);
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

std::ostream& operator<< (std::ostream& out, const std::vector<uint8_t>& v) {
    std::ios_base::fmtflags outf( out.flags() ); // save cout flags
    out << "[" << std::hex;
    size_t last = v.size() - 1;
    for(size_t i = 0; i < v.size(); ++i) {
        out << setw(2) << setfill('0') << static_cast<int>(v[i]);
        if (i != last)
            out << ", ";
    }
    out << "]";
    out.flags(outf); // restore old flags
    return out;
}

std::string generate_id(){
  const auto timestamp{chrono::system_clock::now().time_since_epoch()};
  std::string tempstring{
      to_string(chrono::duration_cast<chrono::milliseconds>(timestamp).count())};
  random_shuffle(tempstring.begin(), tempstring.end());
  return tempstring;
}

// safeGetline to handle \n or \r\n from Stackoverflow User user763305
// https://stackoverflow.com/questions/6089231/getting-std-ifstream-to-handle-lf-cr-and-crlf#6089413
std::istream& safeGetline(std::istream& is, std::string& t)
{
    t.clear();

    // The characters in the stream are read one-by-one using a std::streambuf.
    // That is faster than reading them one-by-one using the std::istream.
    // Code that uses streambuf this way must be guarded by a sentry object.
    // The sentry object performs various tasks,
    // such as thread synchronization and updating the stream state.

    std::istream::sentry se(is, true);
    std::streambuf* sb = is.rdbuf();

    for(;;) {
        int c = sb->sbumpc();
        switch (c) {
        case '\n':
            return is;
        case '\r':
            if(sb->sgetc() == '\n'){
                sb->sbumpc();
            }
            return is;
        case std::streambuf::traits_type::eof():
            // Also handle the case when the last line has no line ending
            if(t.empty()){
                is.setstate(std::ios::eofbit);
            }
            return is;
        default:
            t += (char)c;
        }
    }
}
vector<string> get_headers(istream& is,const string& header){
  vector<string> responsevec;
  string line;
  while(safeGetline(is,line)){
    responsevec.emplace_back(line);
  }
  vector<string> headers;
  for(const auto& line : responsevec){
    if(auto pos = line.find(header); pos != string::npos){
      headers.emplace_back(line.substr(header.length()+2));
    }
  }
  return headers;
}
} // namespace sel
