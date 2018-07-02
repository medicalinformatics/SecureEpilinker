/**
\file    jsonmethodhandler.h
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
\brief Parses and validates requests and generates JSON object from body
*/

#ifndef JSONMETHODHANDLER_H
#define JSONMETHODHANDLER_H
#pragma once

#include <memory>
#include <string>
#include "methodhandler.hpp"
#include "restbed"
#include "resttypes.h"
//#include "seltypes.h"
#include "serverhandler.h"
#include "valijson/validation_results.hpp"

// Forward Declarations
namespace sel {
class Validator;
class ConfigurationHandler;
class ConnectionHandler;

class JsonMethodHandler : public MethodHandler {
  /**
   * Handles Requests with JSON Data
   */
 public:
  explicit JsonMethodHandler(
      const std::string& method,
      std::shared_ptr<ConfigurationHandler> configuration_handler,
      std::shared_ptr<ConnectionHandler> connection_handler,
      std::shared_ptr<ServerHandler> server_handler)
      : MethodHandler(method),
        m_configuration_handler(move(configuration_handler)),
        m_connection_handler(move(connection_handler)),
        m_server_handler(move(server_handler)) {}

  explicit JsonMethodHandler(
      const std::string& method,
      std::shared_ptr<ConfigurationHandler> configuration_handler,
      std::shared_ptr<ConnectionHandler> connection_handler,
      std::shared_ptr<ServerHandler> server_handler,
      std::shared_ptr<Validator> validator)
      : MethodHandler(method, validator),
        m_configuration_handler(move(configuration_handler)),
        m_connection_handler(move(connection_handler)),
        m_server_handler(move(server_handler)) {}

  ~JsonMethodHandler(){};

  void handle_method(std::shared_ptr<restbed::Session>) const override;

  void handle_continue(std::shared_ptr<restbed::Session>) const;

  void use_data(const std::shared_ptr<restbed::Session>&,
                const nlohmann::json&,
                const std::string&) const;

  void set_valid_callback(std::function<SessionResponse(
                              const nlohmann::json&,
                              const std::string&,
                              const std::shared_ptr<ConfigurationHandler>&,
                              const std::shared_ptr<ServerHandler>&,
                              const std::shared_ptr<ConnectionHandler>&)> fun) {
    m_valid_callback = fun;
  }

  void set_invalid_callback(
      std::function<SessionResponse(valijson::ValidationResults&)> fun) {
    m_invalid_callback = fun;
  }

 private:
  std::shared_ptr<ConfigurationHandler> m_configuration_handler;
  std::shared_ptr<ConnectionHandler> m_connection_handler;
  std::shared_ptr<ServerHandler> m_server_handler;

  std::function<SessionResponse(const nlohmann::json&,
                                const std::string&,
                                const std::shared_ptr<ConfigurationHandler>&,
                                const std::shared_ptr<ServerHandler>&,
                                const std::shared_ptr<ConnectionHandler>&)>
      m_valid_callback{nullptr};

  std::function<SessionResponse(valijson::ValidationResults&)>
      m_invalid_callback{nullptr};
};

}  // namespace sel

#endif  // JSONMETHODHANDLER_H
