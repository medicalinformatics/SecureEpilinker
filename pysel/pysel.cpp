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
#include "../include/epilink_result.hpp"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace sel;

PYBIND11_MODULE(pysel, m) {
  m.doc() = "Clear-text EpiLinker";

  // This creates bindings for the Input struct. Record and VRecord can directly
  // be created from python, since they are built from stl types:
  // Record  = map<string, optional<vector<uint8_t>>>
  // VRecord = map<string, vector<optional<vector<uint8_t>>>>
  py::class_<clear_epilink::Input>(m, "Input")
    .def(py::init<const Record&, const VRecord&>());

  py::class_<EpilinkConfig>(m, "EpilinkConfig")
    .def(py::init<
          std::map<FieldName, FieldSpec>,
          std::vector<IndexSet>,
          double, double
        >());

  py::class_<CircuitConfig>(m, "CircuitConfig")
    .def(py::init<const EpilinkConfig&>());

  py::class_<Result<CircUnit>>(m, "ResultInt")
    .def_readonly("index", &Result<CircUnit>::index)
    .def_readonly("match", &Result<CircUnit>::match)
    .def_readonly("tmatch", &Result<CircUnit>::tmatch)
    .def_readonly("sum_field_weights", &Result<CircUnit>::sum_field_weights)
    .def_readonly("sum_weights", &Result<CircUnit>::sum_weights);
  m.def("calc_integer", &clear_epilink::calc_integer,
      "Calculates the EpiLink score using the 32-bit fixed-point circuit.");

  py::class_<Result<double>>(m, "ResultDouble")
    .def_readonly("index", &Result<double>::index)
    .def_readonly("match", &Result<double>::match)
    .def_readonly("tmatch", &Result<double>::tmatch)
    .def_readonly("sum_field_weights", &Result<double>::sum_field_weights)
    .def_readonly("sum_weights", &Result<double>::sum_weights);
  m.def("calc_exact", &clear_epilink::calc_exact,
      "Calculates the EpiLink score using double-precision floats.");
}
