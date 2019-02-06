/**
 \file    statsprinter.cpp
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

#include <chrono>
#include <ctime>
#include <iomanip>
#include "abycore/aby/abyparty.h"
#include "abycore/circuit/arithmeticcircuits.h"
#include "abycore/circuit/booleancircuits.h"
#include "abycore/sharing/sharing.h"
#include "statsprinter.h"

auto constexpr SEP = " = ";

using namespace std::chrono; // for millisecond timings
using namespace std;

namespace sel::aby {

StatsPrinter::StatsPrinter(ABYParty& party) :
  party{party}
{}

void StatsPrinter::set_output(const std::filesystem::path& filepath) {
  if (filepath.empty() || filepath == "-") {
    out = &cout;
    return;
  }

  fout = ofstream(filepath, ios::ate | ios::app);
  out = &fout;
}

void StatsPrinter::set_output(std::ostream* out) {
  this->out = out;
}

auto get_millis() {
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

string now_rfc3339() {
  const auto now = system_clock::now();
  const auto millis = duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000;
  const auto c_now = system_clock::to_time_t(now);

  stringstream ss;
  ss << put_time(gmtime(&c_now), "%FT%T") <<
    '.' << setfill('0') << setw(3) << millis << 'Z';
  return ss.str();
}

void StatsPrinter::print_baseOTs() {
  *out << "[baseOTs]"
    << "\ntime" << SEP << party.GetTiming(P_BASE_OT)
    << "\nsent" << SEP << party.GetSentData(P_BASE_OT)
    << "\nrecv" << SEP << party.GetReceivedData(P_BASE_OT)
    << endl;
}

void StatsPrinter::print_circuit() {
  const auto sharings = party.GetSharings();
  auto as  = sharings[S_ARITH];
  ArithmeticCircuit* ac = (ArithmeticCircuit*) as->GetCircuitBuildRoutine();
  auto bs = sharings[S_BOOL];
  BooleanCircuit* bc = (BooleanCircuit*) bs->GetCircuitBuildRoutine();
  auto ys = sharings[S_YAO];
  BooleanCircuit* yc = (BooleanCircuit*) ys->GetCircuitBuildRoutine();

  *out << "[circuit]"
    << "\ntotal" << SEP << party.GetTotalGates()
    << "\nrounds" << SEP << party.GetTotalDepth();

  *out << "\n[circuit.Arithmetic]"
    << "\nMUL" << SEP << ac->GetNumMULGates()
    << "\nB2A" << SEP << ac->GetNumCONVGates()
    //<< "\nComb" << SEP << ac->GetNumCombGates()
    << "\ntotal" << SEP << ac->GetNumGates()
    << "\nrounds" << SEP << ac->GetMaxDepth();

  *out << "\n[circuit.GMW]"
    << "\nAND" << SEP << bc->GetNumANDGates()
    << "\nXOR" << SEP << bc->GetNumXORVals()
    //<< "\nComb" << SEP << bc->GetNumCombGates()
    << "\ntotal" << SEP << bc->GetNumGates()
    << "\nrounds" << SEP << bc->GetMaxDepth();

  *out << "\n[circuit.Yao]"
    << "\nAND" << SEP << yc->GetNumANDGates()
    << "\nXOR" << SEP << yc->GetNumXORVals()
    << "\nA2Y" << SEP << yc->GetNumA2YGates()
    << "\nB2Y" << SEP << yc->GetNumB2YGates()
    //<< "\nComb" << SEP << yc->GetNumCombGates()
    << "\ntotal" << SEP << yc->GetNumGates()
    << "\nrounds" << SEP << yc->GetMaxDepth()
    << endl;
}

void StatsPrinter::print_communication() {
  *out << "[communication]"
    << "\nsetupCommSent" << SEP << party.GetSentData(P_SETUP)
    << "\nsetupCommRecv" << SEP << party.GetReceivedData(P_SETUP)
    << "\nonlineCommSent" << SEP << party.GetSentData(P_ONLINE)
    << "\nonlineCommRecv" << SEP << party.GetReceivedData(P_ONLINE)
    << endl;
}

void StatsPrinter::print_timings() {
  *out << "[[timings]]"
    << "\ntimestamp" << SEP << now_rfc3339()
    << "\nsetup" << SEP << party.GetTiming(P_SETUP)
    << "\nonline" << SEP << party.GetTiming(P_ONLINE)
    << endl;
}

void StatsPrinter::print_all() {
  print_baseOTs();
  print_circuit();
  print_communication();
  print_timings();
}

void StatsPrinter::print_smart() {
  if (!static_data_printed) {
    print_baseOTs();
    print_circuit();
    print_communication();
    static_data_printed = true;
  }
  print_timings();
}

} // namespace sel::aby
