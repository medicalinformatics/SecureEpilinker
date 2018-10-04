/**
\file    jsonhandlerfunctions.h
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
\brief Functions for handling given JSON Objects
*/

#ifndef SEL_JSONHANDLERFUNCTIONS_H
#define SEL_JSONHANDLERFUNCTIONS_H
#pragma once

#include <memory>
#include "nlohmann/json.hpp"
#include "resttypes.h"
#include "valijson/validation_results.hpp"

namespace sel {

SessionResponse valid_test_config_json_handler(
    const nlohmann::json& j,
    const RemoteId& remote_id,
    const std::string&);

SessionResponse valid_init_local_json_handler(
    const nlohmann::json&,
    const RemoteId&,
    const std::string&);

SessionResponse valid_init_remote_json_handler(
    const nlohmann::json&,
    const RemoteId&,
    const std::string&);

SessionResponse valid_linkrecord_json_handler(
    const nlohmann::json&,
    const RemoteId&,
    const std::string&);

SessionResponse linkrecords(
    const nlohmann::json&,
    const RemoteId&,
    const std::string&,
    bool);

SessionResponse valid_linkrecords_json_handler(
    const nlohmann::json&,
    const RemoteId&,
    const std::string&);

SessionResponse invalid_json_handler(valijson::ValidationResults&);

}  // namespace sel
#endif  // SEL_JSONHANDLERFUNCTIONS_H
