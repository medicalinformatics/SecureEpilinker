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

std::string print_aby_config(const SecureEpilinker::ABYConfig& conf) {
  return ((conf.role == SERVER) ? "Server Config\n"s : "Client Config:\n"s) +
    "Sharing: "s + ((conf.bool_sharing == S_YAO) ? "Yao\n"s : "GMW\n"s ) +
    "Remote Host: "s + conf.remote_host + ":"s + to_string(conf.port) + "\n"s +
    "Threads: "s + to_string(conf.nthreads) + '\n';

}

string print_epilink_config(const EpilinkConfig& conf){
  string returnstring{"Epilink Configuration\n"};
  returnstring += "Bitmask size (in Bit):\t"s + to_string(conf.size_bitmask) + '\n';
  returnstring += "Threshold match: "s + to_string(conf.threshold) + '\n';
  returnstring += "Threshold tentatice match: " + to_string(conf.tthreshold) + '\n';
  returnstring += "Number of bitmask field weights: " + to_string(conf.hw_weights.size()) + '\n';
  returnstring += "Number of binary field weights: " + to_string(conf.bin_weights.size()) + '\n';
  return returnstring;
}
string print_epilink_input(const EpilinkServerInput& in){
  return "Server Input printing not ready yet!\n";
}
string print_epilink_input(const EpilinkClientInput& in){
  string returnstring{"Client Input\n-----\nBitmask Records:\n-----\n"};
  for (size_t i = 0; i != in.hw_record.size(); ++i){
    for(const auto& byte : in.hw_record[i]) {
      returnstring += to_string(byte)+' ';
    }
    returnstring += "Empty: "s+(in.hw_rec_empty[i]? "Yes\n" : "No\n");
  }
  returnstring += "-----\nBinary Records\n-----\n";
  for (size_t i = 0; i != in.bin_record.size(); ++i){
    returnstring += to_string(in.bin_record[i])+" Empty: "+(in.bin_rec_empty[i]? "Yes\n" : "No\n");
  }
  return returnstring + "Number of database records: " + to_string(in.nvals) +'\n';
}
} // namespace sel
