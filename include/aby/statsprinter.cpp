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

  fout = ofstream(filepath, ios::ate | ios::out);
  out = &fout;
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
  *out << "[BaseOTs]"
    << "\nTime" << SEP << party.GetTiming(P_BASE_OT)
    << "\nSent" << SEP << party.GetSentData(P_BASE_OT)
    << "\nRecv" << SEP << party.GetReceivedData(P_BASE_OT)
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

  *out << "[Circuit]"
    << "\nTotal" << SEP << party.GetTotalGates()
    << "\nRounds" << SEP << party.GetTotalDepth();

  *out << "\n[Circuit.Arithmetic]"
    << "\nMUL" << SEP << ac->GetNumMULGates()
    << "\nB2A" << SEP << ac->GetNumCONVGates()
    //<< "\nComb" << SEP << ac->GetNumCombGates()
    << "\nTotal" << SEP << ac->GetNumGates()
    << "\nRounds" << SEP << ac->GetMaxDepth();

  *out << "\n[Circuit.GMW]"
    << "\nAND" << SEP << bc->GetNumANDGates()
    << "\nXOR" << SEP << bc->GetNumXORVals()
    //<< "\nComb" << SEP << bc->GetNumCombGates()
    << "\nTotal" << SEP << bc->GetNumGates()
    << "\nRounds" << SEP << bc->GetMaxDepth();

  *out << "\n[Circuit.Yao]"
    << "\nAND" << SEP << yc->GetNumANDGates()
    << "\nXOR" << SEP << yc->GetNumXORVals()
    << "\nA2Y" << SEP << yc->GetNumA2YGates()
    << "\nB2Y" << SEP << yc->GetNumB2YGates()
    //<< "\nComb" << SEP << yc->GetNumCombGates()
    << "\nTotal" << SEP << yc->GetNumGates()
    << "\nRounds" << SEP << yc->GetMaxDepth()
    << endl;
}

void StatsPrinter::print_communication() {
  *out << "[Communication]"
    << "\nSetupCommSent" << SEP << party.GetSentData(P_SETUP)
    << "\nSetupCommRecv" << SEP << party.GetReceivedData(P_SETUP)
    << "\nOnlineCommSent" << SEP << party.GetSentData(P_ONLINE)
    << "\nOnlineCommRecv" << SEP << party.GetReceivedData(P_ONLINE)
    << endl;
}

void StatsPrinter::print_timings() {
  *out << "[[Timings]]"
    << "\nTimestamp" << SEP << now_rfc3339()
    << "\nSetup" << SEP << party.GetTiming(P_SETUP)
    << "\nOnline" << SEP << party.GetTiming(P_ONLINE)
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
