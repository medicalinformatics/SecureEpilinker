/**
\file    headermethodhandler.h
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
\brief handles requests without data payload but relevant headers
*/

#ifndef SEL_HEADERMETHODHANDLER_H
#define SEL_HEADERMETHODHANDLER_H
#pragma once

#include <memory>
#include <string>
#include "methodhandler.hpp"
#include "restbed"
#include "resttypes.h"

// Forward Declarations
namespace spdlog {
class logger;
}

namespace sel {
class ConfigurationHandler;
class ConnectionHandler;
class DataHandler;
class ServerHandler;

class HeaderMethodHandler : public MethodHandler {
 public:
HeaderMethodHandler(
      const std::string& method,
      std::shared_ptr<ConfigurationHandler> config_handler,
      std::shared_ptr<ConnectionHandler> connection_handler,
      std::shared_ptr<ServerHandler> server_handler,
      std::shared_ptr<DataHandler> data_handler, 
      std::function<SessionResponse(
        const std::shared_ptr<restbed::Session>&, 
        const std::shared_ptr<const restbed::Request>&,
        const std::multimap<std::string,std::string>&,
        const std::string&,
        const std::shared_ptr<ConfigurationHandler>&,
        const std::shared_ptr<ConnectionHandler>&,
        const std::shared_ptr<ServerHandler>&,
        const std::shared_ptr<DataHandler>&,
        const std::shared_ptr<spdlog::logger>&)> = nullptr);
  ~HeaderMethodHandler() = default;
  void handle_method(std::shared_ptr<restbed::Session>) const override;
  void set_handling_function(std::function<SessionResponse(
        const std::shared_ptr<restbed::Session>&, 
        const std::shared_ptr<const restbed::Request>&,
        const std::multimap<std::string,std::string>&,
        const std::string&,
        const std::shared_ptr<ConfigurationHandler>&,
        const std::shared_ptr<ConnectionHandler>&,
        const std::shared_ptr<ServerHandler>&,
        const std::shared_ptr<DataHandler>&,
        const std::shared_ptr<spdlog::logger>&)>);

 private:
  std::shared_ptr<ConfigurationHandler> m_config_handler;
  std::shared_ptr<ConnectionHandler> m_connection_handler;
  std::shared_ptr<ServerHandler> m_server_handler;
  std::shared_ptr<DataHandler> m_data_handler;
  std::shared_ptr<spdlog::logger> m_logger;
  std::function<SessionResponse(
                                const std::shared_ptr<restbed::Session>&, 
                                const std::shared_ptr<const restbed::Request>&,
                                const std::multimap<std::string,std::string>&,
                                const std::string&,
                                const std::shared_ptr<ConfigurationHandler>&,
                                const std::shared_ptr<ConnectionHandler>&,
                                const std::shared_ptr<ServerHandler>&,
                                const std::shared_ptr<DataHandler>&,
                                const std::shared_ptr<spdlog::logger>&
                                )> m_handling_function{nullptr};
};

}  // namespace sel

#endif  // SEL_HEADERMETHODHANDLER_H
