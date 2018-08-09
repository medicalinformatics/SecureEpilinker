/**
\file    headerhandlerfunctions.h
\author  Tobias Kussel <kussel@cbs.tu-darmstadt.de>
\copyright SEL - Secure EpiLinker
    Copyright (C) 2018 Computational Biology & Simulation Group TU-Darmstadt
This program is free software: you can redistribute it and/or modify it under
the terms of the GNU Affero General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option) any
later version. This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Affero General Public License for more details.
    You should have received a copy of the GNU Affero General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.  
\brief Containes functions for the HeaderMethodHander
*/

#include <string>
#include <map>
#include "resttypes.h"

namespace restbed{
  class Session;
  class Request;
}

namespace spdlog{
  class logger;
}

namespace sel {
  class ConfigurationHandler;
  class ConnectionHandler;
  class ServerHandler;
  class DataHandler;

SessionResponse temp_sel_init(const std::shared_ptr<restbed::Session>&,
                              const std::shared_ptr<const restbed::Request>&,
                              const std::multimap<std::string,std::string>& headers, 
                              std::string remote_id,
                              const std::shared_ptr<ConfigurationHandler>&,
                              const std::shared_ptr<ConnectionHandler>& connection_handler,
                              const std::shared_ptr<ServerHandler>&,
                              const std::shared_ptr<DataHandler>&,
                              const std::shared_ptr<spdlog::logger>& logger);
SessionResponse test_config(const std::shared_ptr<restbed::Session>&,
                              const std::shared_ptr<const restbed::Request>&,
                              const std::multimap<std::string,std::string>& headers, 
                              std::string remote_id,
                              const std::shared_ptr<ConfigurationHandler>&,
                              const std::shared_ptr<ConnectionHandler>& connection_handler,
                              const std::shared_ptr<ServerHandler>&,
                              const std::shared_ptr<DataHandler>&,
                              const std::shared_ptr<spdlog::logger>& logger);
SessionResponse init_mpc(const std::shared_ptr<restbed::Session>&,
                              const std::shared_ptr<const restbed::Request>&,
                              const std::multimap<std::string,std::string>& headers, 
                              std::string remote_id,
                              const std::shared_ptr<ConfigurationHandler>&,
                              const std::shared_ptr<ConnectionHandler>&,
                              const std::shared_ptr<ServerHandler>& server_handler,
                              const std::shared_ptr<DataHandler>& data_handler,
                              const std::shared_ptr<spdlog::logger>& logger);
} // namespace sel