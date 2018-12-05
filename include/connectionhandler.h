/**
\file    connectionhandler.h
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
\brief Holds the connections to every remote host and dispatches ABY
computations
*/

#ifndef SEL_CONNECTIONHANDLER_H
#define SEL_CONNECTIONHANDLER_H
#pragma once

#include "localconfiguration.h"
#include "configurationhandler.h"
#include <memory>
#include <unordered_map>
#include <mutex>
#include "util.h"
#include "resttypes.h"

namespace restbed {
class Service;
}

namespace sel {
class RemoteConfiguration;
class LinkageJob;
class AuthenticationConfig;
//class LocalConfiguration;

  struct RemoteAddress {
    std::string ip;
    Port port;
    explicit RemoteAddress(std::string url) {
      auto temp = split(url,':');
      port = std::stoul(temp.back());
      ip = temp.front();
      if(ip == "localhost")
        ip = "127.0.0.1";
    }
    RemoteAddress(std::string ip, Port port) : ip(ip), port(port) {}
  };
class ConnectionHandler {
  /**
   * Handle connection configurations and jobs. Dispatch ABY computations
   */
  protected:
    ConnectionHandler() = default;
 public:
  static  ConnectionHandler& get();
  static  ConnectionHandler const& cget();
  void set_service(restbed::Service*);
  bool connection_exists(const RemoteId& c_id) const;

  void populate_aby_ports();

  const FieldSpec& get_field(const FieldName& name);

  std::shared_ptr<restbed::Service> get_service() const;

  Port use_free_port();
  std::set<Port> get_free_ports() const;
  Port choose_aby_port();
  void mark_port_used(Port);

  Port initialize_aby_server(std::shared_ptr<RemoteConfiguration>);

 private:
  std::shared_ptr<restbed::Service> m_service;
  std::set<Port> m_aby_available_ports;
  std::mutex m_port_mutex;
};

}  // Namespace sel

#endif  // SEL_CONNECTIONHANDLER_
