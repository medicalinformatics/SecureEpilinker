/**
 \file    util.cpp
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
 \brief ABY utilities
*/

#include <chrono>
#include <functional>
#include "abycore/aby/abyparty.h"
#include "util.h"
#include "circuit_defs.h"

#define SEP "\t"

using namespace std::chrono; // for millisecond timings
using namespace std;

namespace sel {

uint32_t get_milis() {
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

/*
 * Prints stats that don't change from query to query
 */
void print_stats_query_oneoff(ABYParty& party) {
  //Sharing* bs = party->GetSharings()[S_BOOL];
  //BooleanCircuit* bc = (BooleanCircuit*) bs->GetCircuitBuildRoutine();
  Sharing* bs = party.GetSharings()[S_ARITH];
  ArithmeticCircuit* bc = (ArithmeticCircuit*) bs->GetCircuitBuildRoutine();
  // Print Gates Statistics
  cout << "#ArithmeticCircuit" << endl
    //<< "NumANDGates" << SEP << bc->GetNumANDGates() << endl
    << "NumMULGates" << SEP << bc->GetNumMULGates() << endl
    << "NumCONVGates" << SEP << bc->GetNumCONVGates() << endl
    << "Depth" << SEP << bs->GetMaxCommunicationRounds() << endl
    << "NumCombGates" << SEP << bc->GetNumCombGates() << endl
    << "TotalGates" << SEP << bc->GetNumGates() << endl; // TODO Let ABYParty expose TotalNumberGates via call to m_pCircuit->GetGateHead()

  // One-off timing and communication
  cout << "BaseOTsTiming" << SEP << party.GetTiming(P_BASE_OT) << endl
    << "SetupCommSent" << SEP << party.GetSentData(P_SETUP) << endl
    << "SetupCommRecv" << SEP << party.GetReceivedData(P_SETUP) << endl
    << "OnlineCommSent" << SEP << party.GetSentData(P_ONLINE) << endl
    << "OnlineCommRecv" << SEP << party.GetReceivedData(P_ONLINE) << endl;

  // Finally print header for timings
  cout << "#QueryTimings" << endl << "Time" << SEP << "Setup" << SEP << "Online" << endl;
}

/*
 * Print timings as CSVs
 * Lets us call this experiment and collect stdout into a file for further
 * stats.
 * Don't forget to #define ABY_PRODUCTION
 */
const ABYPHASE STAT_PHASES[] = {P_SETUP, P_ONLINE};
void print_stats_query(ABYParty& party) {
  // print one-off stats and header on first call
  static bool oneoff_printed = false;
  if (!oneoff_printed) {
    print_stats_query_oneoff(party);
    oneoff_printed = true;
  }

  cout << get_milis(); // end time in ms, so we can join concurrent threads
  for (ABYPHASE phase: STAT_PHASES) {
    cout << SEP << party.GetTiming(phase);
  }
  cout << endl;

}

} // namespace sel
