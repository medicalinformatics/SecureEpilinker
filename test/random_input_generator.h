/**
 \file    test/random_input_generator.h
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

#ifndef SEL_TEST_RANDOM_INPUT_GENERATOR_H
#define SEL_TEST_RANDOM_INPUT_GENERATOR_H
#pragma once

#include <random>
#include "../include/epilink_input.h"

namespace sel::test {

struct EpilinkInput {
  EpilinkConfig cfg;
  EpilinkClientInput client;
  EpilinkServerInput server;
};

class RandomInputGenerator {
public:
  RandomInputGenerator(const EpilinkConfig& cfg);
  RandomInputGenerator(EpilinkConfig&& cfg);

  /* Generate random input, moving the PRNG state forward */
  EpilinkInput generate(const size_t nvals);

  /**
    * The bitmask density shift controls the density of randomly set bits in
    * the bitmasks:
    *   = 0 - equal number of 1s and 0s
    *   > 0 - more 1s than 0s
    *   < 0 - less 1s than 0s
    */
  void set_bitmask_density_shift(int shift);

  /**
    * Binary match probability gives the random probability with which a
    * database binary field is set to the corresponding client record entry.
    * Otherwise the probability of a match for randomly generated binary fields
    * would diminishingly low.
    */
  void set_binary_match_probability(double prob);

  /**
    * Fields to set empty for client input.
    */
  void set_client_empty_fields(const std::vector<FieldName>& empty_fields);

  /**
    * Probability with which a field is set to be empty for the server input.
    */
  void set_server_empty_field_probability(double prob);

private:
const EpilinkConfig cfg;
  int bm_density_shift = 0;
  double bin_match_prob = .5;
  double server_empty_field_prob = .2;
  std::vector<FieldName> client_empty_fields;

  std::mt19937 gen{73};
  std::uniform_int_distribution<> random_byte{0, 0xff};
  std::bernoulli_distribution random_match{bin_match_prob};
  std::bernoulli_distribution random_empty{server_empty_field_prob};

  Bitmask random_bm(const size_t bitsize, const int density_shift);
};

} /* END namespace sel::test */

#endif /* end of include guard: SEL_TEST_RANDOM_INPUT_GENERATOR_H */
