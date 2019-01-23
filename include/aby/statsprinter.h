/**
 \file    statsprinter.h
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
 \brief ABY statistics printer
*/

#ifndef SEL_ABY_STATSPRINTER_H_
#define SEL_ABY_STATSPRINTER_H_
#pragma once

#include <iostream>
#include <fstream>
#include <filesystem>

// forward declarations
class ABYParty;
//namespace std::filesystem {
//class path;
//}

namespace sel::aby {

/**
 * An attempt to bring some sanity to the statistics output of ABY
 *
 * Danger: Some gate numbers are according to the number of bits or values
 * (arith.), that is, without SIMD multiplicity, i.e., nvals, taken into account.
 * But some are according to the number of abstract gates.
 * Per bit/value: MUL, AND, XOR, conversions (B2A per bits!)
 * Per abstract gate: Totals, GetNumXORGates (GetNumXORVals is per bit),
 *    structural gates
 * Also, circuit depth is really the number of interactive communication
 * rounds, which is similar to depth for A and B, but Y only ever has 2 or 3
 * such rounds
 */
class StatsPrinter {
public:
  StatsPrinter(ABYParty& party);

  /**
   * set_output sets the output of stats printing.
   * If empty, or "-", output is written to stdout.
   */
  void set_output(const std::filesystem::path& filepath = std::filesystem::path());

  void print_baseOTs();
  void print_circuit();
  void print_communication();
  void print_timings();
  void print_all();
  /**
   * Prints BaseOTs, circuit and communication stats only on first call,
   * but timings on every call.
   * This makes only sense on consecutive runs using the _same circuit layout_.
   * If the circuit changes between runs, all, but the BaseOTs stats, should be
   * printed again.
   */
  void print_smart();

private:
  ABYParty& party;
  std::ostream* out = &std::cout;
  std::ofstream fout;
  bool static_data_printed = false;
};

} // namespace sel::aby

#endif /* SEL_ABY_STATSPRINTER_H_ */
