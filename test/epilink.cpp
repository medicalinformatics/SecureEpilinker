/**
 \file    epilink.h
 \author  Sebastian Stammler <sebastian.stammler@cysec.de>
 \copyright SEL - Secure EpiLinker
      Copyright (C) 2020 Computational Biology & Simulation Group TU-Darmstadt
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
 \brief Epilink-related testing utilities
*/

#include "epilink.h"

namespace sel::test {

EpilinkConfig make_dkfz_cfg() {
  return {
    { // begin map<string, ML_Field>
      { "vorname",
        FieldSpec("vorname", 0.000235, 0.01, "dice", "bitmask", 500) },
      { "nachname",
        FieldSpec("nachname", 0.0000271, 0.008, "dice", "bitmask", 500) },
      { "geburtsname",
        FieldSpec("geburtsname", 0.0000271, 0.008, "dice", "bitmask", 500) },
      { "geburtstag",
        FieldSpec("geburtstag", 0.0333, 0.005, "binary", "integer", 5) },
      { "geburtsmonat",
        FieldSpec("geburtsmonat", 0.0833, 0.002, "binary", "integer", 4) },
      { "geburtsjahr",
        FieldSpec("geburtsjahr", 0.0286, 0.004, "binary", "integer", 11) },
      { "plz",
        FieldSpec("plz", 0.01, 0.04, "binary", "string", 40) },
      { "ort",
        FieldSpec("ort", 0.01, 0.04, "dice", "bitmask", 500) }
    }, // end map<string, ML_Field>
    { { "vorname", "nachname", "geburtsname" } }, // exchange groups
    Threshold, TThreshold
  };
}

} /* END namespace sel::test */
