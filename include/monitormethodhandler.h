/**
\file    monitormethodhandler.h
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
\brief Handles job status monitoring requests and returns status
*/

#ifndef SEL_MONITORMETHODHANDLER_H
#define SEL_MONITORMETHODHANDLER_H
#pragma once

#include "methodhandler.hpp"
#include "seltypes.h"
#include "valijson/validation_results.hpp"
#include "restbed"
#include <memory>
#include <string>

// Forward Declarations

namespace sel {
class Validator;
class ConnectionHandler;
class LinkageJob;

class MonitorMethodHandler : public MethodHandler {
  /**
   * Handles Job Monitoring Requests
   */
 public:
  explicit MonitorMethodHandler(
      const std::string& method,
      std::shared_ptr<ConnectionHandler> connection_handler)
      : MethodHandler(method), m_connection_handler(connection_handler){}
  explicit MonitorMethodHandler(
      const std::string& method,
      std::shared_ptr<ConnectionHandler> connection_handler,
      std::shared_ptr<Validator> validator)
      : MethodHandler(method, validator),
        m_connection_handler(connection_handler){}
  ~MonitorMethodHandler() = default;
  void handle_method(std::shared_ptr<restbed::Session>) const override;

 private:
  std::shared_ptr<ConnectionHandler> m_connection_handler;

};

}  // namespace sel

#endif  // SEL_MONITORMETHODHANDLER_H
