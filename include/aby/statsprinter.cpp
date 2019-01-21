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

void StatsPrinter::print_circuit() {
  const auto sharings = party.GetSharings();
  auto as  = sharings[S_ARITH];
  ArithmeticCircuit* ac = (ArithmeticCircuit*) as->GetCircuitBuildRoutine();
  auto bs = sharings[S_BOOL];
  BooleanCircuit* bc = (BooleanCircuit*) bs->GetCircuitBuildRoutine();
  auto ys = sharings[S_YAO];
  BooleanCircuit* yc = (BooleanCircuit*) ys->GetCircuitBuildRoutine();

  *out << "[Circuit.Arithmetic]"
    << "\nMUL" << SEP << ac->GetNumMULGates()
    << "\nNonLinear" << SEP << as->GetNumNonLinearOperations()
    << "\nCONV" << SEP << ac->GetNumCONVGates()
    << "\nComb" << SEP << ac->GetNumCombGates()
    << "\nTotal" << SEP << ac->GetNumGates()
    << "\nMaxDepth" << SEP << ac->GetMaxDepth()
    << "\nDepth" << SEP << as->GetMaxCommunicationRounds();

  *out << "\n[Circuit.GMW]"
    << "\nAND" << SEP << bc->GetNumANDGates()
    << "\nNonLinear" << SEP << bs->GetNumNonLinearOperations()
    << "\nXOR" << SEP << bc->GetNumXORGates()
    << "\nA2Y" << SEP << bc->GetNumA2YGates()
    << "\nB2Y" << SEP << bc->GetNumB2YGates()
    //<< "\nConv" << SEP << bc->GetNumCONVGates()
    << "\nComb" << SEP << bc->GetNumCombGates()
    << "\nTotal" << SEP << bc->GetNumGates()
    << "\nMaxDepth" << SEP << bc->GetMaxDepth()
    << "\nDepth" << SEP << bs->GetMaxCommunicationRounds();

  *out << "\n[Circuit.Yao]"
    << "\nAND" << SEP << yc->GetNumANDGates()
    << "\nNonLinear" << SEP << ys->GetNumNonLinearOperations()
    << "\nXOR" << SEP << yc->GetNumXORGates()
    << "\nA2Y" << SEP << yc->GetNumA2YGates()
    << "\nB2Y" << SEP << yc->GetNumB2YGates()
    //<< "\nConv" << SEP << yc->GetNumCONVGates()
    << "\nComb" << SEP << yc->GetNumCombGates()
    << "\nTotal" << SEP << yc->GetNumGates()
    << "\nMaxDepth" << SEP << yc->GetMaxDepth()
    << "\nDepth" << SEP << ys->GetMaxCommunicationRounds()
    << endl;

  // TODO Implement
  // ABYParty::GetTotalGates() {return m_pCircuit->GetGateHead();}
  // ABYParty::GetTotalDepth() {return m_pCircuit->GetTotalDepth();}
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
    << "\nBaseOTs" << SEP << party.GetTiming(P_BASE_OT)
    << "\nSetup" << SEP << party.GetTiming(P_SETUP)
    << "\nOnline" << SEP << party.GetTiming(P_SETUP)
    << endl;
}

void StatsPrinter::print_all() {
  print_circuit();
  print_communication();
  print_timings();
}

void StatsPrinter::print_smart() {
  if (!static_data_printed) {
    print_circuit();
    print_communication();
    static_data_printed = true;
  }
  print_timings();
}

} // namespace sel::aby
