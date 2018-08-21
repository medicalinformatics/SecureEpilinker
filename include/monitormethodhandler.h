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
//#include "seltypes.h"
#include <memory>
#include <string>
#include "restbed"
#include "resttypes.h"
#include "valijson/validation_results.hpp"

// Forward Declarations

namespace spdlog {
class logger;
}
namespace sel {
class Validator;
class ConfigurationHandler;
class LinkageJob;
class ServerHandler;

class MonitorMethodHandler : public MethodHandler {
  /**
   * Handles Job Monitoring Requests
   */
 public:
  MonitorMethodHandler(const std::string& method);
  MonitorMethodHandler(const std::string& method, std::shared_ptr<Validator>& validator);
  ~MonitorMethodHandler() = default;
  void handle_method(std::shared_ptr<restbed::Session>) const override;

 private:
  std::shared_ptr<spdlog::logger> m_logger;
};

}  // namespace sel

#endif  // SEL_MONITORMETHODHANDLER_H
