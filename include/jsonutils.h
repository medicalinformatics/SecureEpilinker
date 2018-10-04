/**
\file    jsonutils.h
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
  \brief utility functions regarding json data
*/

#ifndef SEL_JSONUTILS_H
#define SEL_JSONUTILS_H
#pragma once

#include "seltypes.h"
#include "epilink_input.h"
#include "nlohmann/json.hpp"
#include <filesystem>

namespace sel {

FieldEntry parse_json_field(const FieldSpec&, const nlohmann::json&);
Record parse_json_fields(const std::map<FieldName, FieldSpec>&,
                         const nlohmann::json&);
VRecord parse_json_fields_array(const std::map<FieldName, FieldSpec>& fields,
                                const nlohmann::json& json);
VRecord record_to_vrecord(const Record&);
std::vector<std::string> parse_json_id_array(const nlohmann::json& json);
std::map<FieldName, FieldSpec> parse_json_fields_config(
    nlohmann::json fields_json);
std::vector<IndexSet> parse_json_exchange_groups(nlohmann::json xgroups_json);
EpilinkConfig parse_json_epilink_config(nlohmann::json config_json);

nlohmann::json read_json_from_disk(const std::filesystem::path&);

} // namespace sel

#endif /* end of include guard: SEL_JSONUTILS_H */
