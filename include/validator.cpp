/**
\file    validator.cpp
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

#include "validator.h"

#include "nlohmann/json.hpp"
#include "valijson/adapters/nlohmann_json_adapter.hpp"
#include "valijson/schema.hpp"
#include "valijson/schema_parser.hpp"
#include "valijson/utils/nlohmann_json_utils.hpp"
#include "valijson/validator.hpp"
#include "fmt/format.h"
#include <tuple>

using json = nlohmann::json;
using valijson::Schema;
using valijson::SchemaParser;
using valijson::ValidationResults;
using valijson::adapters::NlohmannJsonAdapter;

sel::Validator::Validator() {
 m_schema = "{}"_json; // Accept everything
}

std::pair<bool, ValidationResults> sel::Validator::validate_json(
    const json& data) const {
  /**
   * Validate JSON schema compatibility and data logic
   */

  Schema json_schema;
  SchemaParser parser;
  NlohmannJsonAdapter schema_doc(m_schema);
  parser.populateSchema(schema_doc, json_schema);

  valijson::Validator validator;
  NlohmannJsonAdapter doc(data);
  ValidationResults results;

  if (!validator.validate(json_schema, doc, &results)) {
    return std::make_pair(false, results);
  }
  return std::make_pair(logic_validation(data),
                        results);  // Does the data make sense?
}

bool sel::Validator::logic_validation(const json& data) const {
  /**
   * Validata data logic
   */
  // TODO(TK) Write logic
  return true;
}
