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
#include "authenticationconfig.hpp"
#include "connectionhandler.h"
#include "nlohmann/json.hpp"
#include "seltypes.h"
#include "resttypes.h"
#include "valijson/validation_results.hpp"

namespace sel {
ServerConfig make_server_config_from_json(nlohmann::json);

std::unique_ptr<AuthenticationConfig> get_auth_object(const nlohmann::json&);

SessionResponse valid_test_config_json_handler(
    const nlohmann::json& j,
    const RemoteId& remote_id,
    const std::shared_ptr<ConfigurationHandler>& config_handler,
    const std::shared_ptr<ServerHandler>& server_handler,
    const std::shared_ptr<ConnectionHandler>&);

SessionResponse valid_init_local_json_handler(
    const nlohmann::json&,
    RemoteId,
    const std::shared_ptr<ConfigurationHandler>&,
    const std::shared_ptr<ServerHandler>&,
    const std::shared_ptr<ConnectionHandler>&);

SessionResponse valid_init_remote_json_handler(
    const nlohmann::json&,
    RemoteId,
    const std::shared_ptr<ConfigurationHandler>&,
    const std::shared_ptr<ServerHandler>&,
    const std::shared_ptr<ConnectionHandler>&);

SessionResponse valid_linkrecord_json_handler(
    const nlohmann::json&,
    const RemoteId&,
    const std::shared_ptr<ConfigurationHandler>&,
    const std::shared_ptr<ServerHandler>&,
    const std::shared_ptr<ConnectionHandler>&);

SessionResponse invalid_json_handler(valijson::ValidationResults&);

}  // namespace sel
#endif  // SEL_JSONHANDLERFUNCTIONS_H
