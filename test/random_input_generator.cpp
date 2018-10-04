/**
 \file    test/random_input_generator.cpp
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
 \brief Generate random Epilink Input for EpilinkConfig
*/

#include "random_input_generator.h"
#include "../include/util.h"

using namespace std;

namespace sel::test {

RandomInputGenerator::RandomInputGenerator(const EpilinkConfig& cfg) :
  cfg{cfg} {}

RandomInputGenerator::RandomInputGenerator(EpilinkConfig&& cfg) :
  cfg{move(cfg)} {}

void RandomInputGenerator::set_bitmask_density_shift(int shift) {
  bm_density_shift = shift;
}

void RandomInputGenerator::set_binary_match_probability(double prob) {
  bin_match_prob = prob;
  random_match = bernoulli_distribution(prob);
}

void RandomInputGenerator::set_client_empty_fields(
    const std::vector<FieldName>& empty_fields) {
  client_empty_fields = empty_fields;
}

void RandomInputGenerator::set_server_empty_field_probability(double prob) {
  server_empty_field_prob = prob;
  random_empty = bernoulli_distribution(prob);
}

Bitmask RandomInputGenerator::random_bm(const size_t bitsize, const int density_shift) {
  Bitmask bm(bitbytes(bitsize));
  for(auto& b : bm) {
    b = random_byte(gen);
    for (int i = 0; i != abs(density_shift); ++i) {
      if (density_shift > 0)  b |= random_byte(gen);
      else if (density_shift < 0)  b &= random_byte(gen);
    }
  }
  // set most significant bits to 0 if bitsize%8
  if (bitsize%8) bm.back() >>= (8-bitsize%8);
  return bm;
}

EpilinkInput RandomInputGenerator::generate(const size_t nvals) {
  // Client Input
  EpilinkClientInput in_client {
    transform_map(cfg.fields, [this](const FieldSpec& f)
      -> FieldEntry {
        bool be_empty = vec_contains(client_empty_fields, f.name);
        if (be_empty) {
          return nullopt;
        } else {
          return random_bm(f.bitsize,
              (f.comparator == FieldComparator::DICE) ? bm_density_shift : 0);
        }
      }),
      nvals
  };

  // Server Input

  EpilinkServerInput in_server {
    transform_map(cfg.fields,
      [this, &nvals, &in_client](const FieldSpec& f)
      -> VFieldEntry {
        VFieldEntry ve;
        ve.reserve(nvals);
        for (size_t i = 0; i < nvals; ++i) {
          if (random_empty(gen)) {
            ve.emplace_back(nullopt);
          } else if (f.comparator == FieldComparator::DICE) {
            ve.emplace_back(random_bm(f.bitsize, bm_density_shift));
          } else if (random_match(gen)) {
            ve.emplace_back(in_client.records.at(f.name));
          } else {
            ve.emplace_back(random_bm(f.bitsize, 0));
          }
        }
        return ve;
      })
  };

  return {cfg, in_client, in_server};

}

} /* END namespace sel */
