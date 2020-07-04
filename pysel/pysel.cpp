/**
 \file    pysel.cpp
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
 \brief Python bindings for clear-text EpiLinker
*/

#include "../include/clear_epilinker.h"
#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(pysel, m) {
  m.doc() = "Clear-text EpiLinker";

  m.def("calc_integer", &sel::clear_epilink::calc_integer,
      "Calculates the EpiLink score using the 32-bit fixed-point circuit.");
  m.def("calc_exact", &sel::clear_epilink::calc_exact,
      "Calculates the EpiLink score using double-precision floats.");
}
