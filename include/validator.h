/**
\file    validator.h
\author  Tobias Kussel <kussel@cbs.tu-darmstadt.de>
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
\brief Validates JSON inputs against json schema data
*/

#ifndef SEL_VALIDATOR_H
#define SEL_VALIDATOR_H
#pragma once

#include "nlohmann/json.hpp"
#include "valijson/validation_results.hpp"
#include <tuple>

namespace sel {

class Validator {
 public:
   Validator();
   explicit Validator(const nlohmann::json& schema) : m_schema(schema) {}
   std::pair<bool, valijson::ValidationResults> validate_json(const nlohmann::json& data) const;
  void set_schema(const nlohmann::json& schema) {m_schema = schema;};
  const nlohmann::json& get_schema() const {return m_schema;}
 private:
  bool logic_validation(const nlohmann::json& data) const;
  nlohmann::json m_schema;
};

}  // Namespace sel

#endif  // SEL_VALIDATOR_H
