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
#include "../include/logger.h"
#include "../test/epilink.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace sel;

auto epilink_dkfz_int(const clear_epilink::Input& input) {
  auto cfg = CircuitConfig(test::make_dkfz_cfg());
  return clear_epilink::calc_integer(input, cfg);
}

auto epilink_dkfz_exact(const clear_epilink::Input& input) {
  auto cfg = CircuitConfig(test::make_dkfz_cfg());
  return clear_epilink::calc_exact(input, cfg);
}

// multi-record versions

auto v_epilink_int(const Records& records, const VRecord& database, const CircuitConfig& cfg) {
  return clear_epilink::calc<CircUnit>(records, database, cfg);
}

auto v_epilink_exact(const Records& records, const VRecord& database, const CircuitConfig& cfg) {
  return clear_epilink::calc<double>(records, database, cfg);
}

auto v_epilink_dkfz_int(const Records& records, const VRecord& database) {
  auto cfg = CircuitConfig(test::make_dkfz_cfg());
  return v_epilink_int(records, database, cfg);
}

auto v_epilink_dkfz_exact(const Records& records, const VRecord& database) {
  auto cfg = CircuitConfig(test::make_dkfz_cfg());
  return v_epilink_exact(records, database, cfg);
}

void set_log_level(int lvl) {
  spdlog::set_level(spdlog::level::level_enum(lvl));
}

PYBIND11_MODULE(pysel, m) {
  m.doc() = "Clear-text EpiLinker";

  // Epilink and Circuit configuration types

  py::class_<FieldSpec>(m, "FieldSpec")
    .def(py::init<
          const std::string&, // name
          double, // frequency
          double, // error
          const std::string&, // comparator
          const std::string&, // type
          const size_t // bitsize
        >())
    .def(py::init<
          const std::string&, // name
          const double, // weight
          const FieldComparator, // comparator
          const FieldType, // type
          const size_t // bitsize
        >());

  py::class_<EpilinkConfig>(m, "EpilinkConfig")
    .def(py::init<
          std::map<FieldName, FieldSpec>, // field weights
          std::vector<IndexSet>, // exchange groups
          double, double // match(,tentative) thresholds
        >());

  // DKFZ Mainzelliste default config
  m.def("dkfz_cfg", &test::make_dkfz_cfg,
      "Returns the default DKFZ Mainzelliste EpiLink configuration.");

  py::class_<CircuitConfig>(m, "CircuitConfig")
    .def(py::init<const EpilinkConfig&>());

  // Epilink Input type. Record and VRecord can directly
  // be created from python, since they are built from stl types:
  // Record  = map<string, optional<vector<uint8_t>>>
  // VRecord = map<string, vector<optional<vector<uint8_t>>>>
  py::class_<clear_epilink::Input>(m, "Input")
    .def(py::init<const Record&, const VRecord&>());

  // Clear-text Epilink function bindings and Return types

  py::class_<Result<CircUnit>>(m, "ResultInt")
    .def_readonly("index", &Result<CircUnit>::index)
    .def_readonly("match", &Result<CircUnit>::match)
    .def_readonly("tmatch", &Result<CircUnit>::tmatch)
    .def_readonly("sum_field_weights", &Result<CircUnit>::sum_field_weights)
    .def_readonly("sum_weights", &Result<CircUnit>::sum_weights);
  m.def("epilink_int", &clear_epilink::calc_integer,
      "Calculates the EpiLink score using the 32-bit fixed-point circuit.");
  m.def("epilink_dkfz_int", &epilink_dkfz_int,
      "Calculates the EpiLink score using the 32-bit fixed-point circuit"
      " with the DKFZ EpiLink configuration.");
  m.def("v_epilink_int", &v_epilink_int,
      "Calculates the EpiLink score using the 32-bit fixed-point circuit. Multi-record version.");
  m.def("v_epilink_dkfz_int", &v_epilink_dkfz_int,
      "Calculates the EpiLink score using the 32-bit fixed-point circuit"
      " with the DKFZ EpiLink configuration. Multi-record version.");

  py::class_<Result<double>>(m, "ResultDouble")
    .def_readonly("index", &Result<double>::index)
    .def_readonly("match", &Result<double>::match)
    .def_readonly("tmatch", &Result<double>::tmatch)
    .def_readonly("sum_field_weights", &Result<double>::sum_field_weights)
    .def_readonly("sum_weights", &Result<double>::sum_weights);
  m.def("epilink_exact", &clear_epilink::calc_exact,
      "Calculates the EpiLink score using double-precision floats.");
  m.def("epilink_dkfz_exact", &epilink_dkfz_exact,
      "Calculates the EpiLink score using double-precision floats"
      " with the DKFZ EpiLink configuration.");
  m.def("v_epilink_exact", &v_epilink_exact,
      "Calculates the EpiLink score using double-precision floats. Multi-record version.");
  m.def("v_epilink_dkfz_exact", &v_epilink_dkfz_exact,
      "Calculates the EpiLink score using double-precision floats"
      " with the DKFZ EpiLink configuration. Multi-record version.");

  // optional configuration of log-level: trace..crit = 0..5, off = 6.
  m.def("set_log_level", &set_log_level,
      "Sets the log-level of the terminal logger: trace..crit = 0..5, off = 6.");

  // module initialization
  create_terminal_logger();
}
