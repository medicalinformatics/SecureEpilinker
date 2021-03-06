/**
\file    connectionhandler.cpp
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

#include "connectionhandler.h"
#include <algorithm>
#include <iterator>
#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>
#include <vector>
#include "localconfiguration.h"
#include "remoteconfiguration.h"
#include "restutils.h"
#include "restbed"

using namespace std;
namespace sel {
ConnectionHandler& ConnectionHandler::get(){
  static ConnectionHandler singleton;
  return singleton;
}

ConnectionHandler const& ConnectionHandler::cget() {
  return cref(get());
}

  void ConnectionHandler::set_service(restbed::Service* service) {
    m_service.reset(service);
  }

shared_ptr<restbed::Service> ConnectionHandler::get_service() const {
  return m_service;
}

Port ConnectionHandler::use_free_port() {
  if (auto it = m_aby_available_ports.begin(); !m_aby_available_ports.empty()) {
    auto port = *it;
    m_aby_available_ports.erase(it);
    return port;
  } else {
    throw std::runtime_error("No remaining port for ABY");
  }
  return 0u;
}

set<Port> ConnectionHandler::get_free_ports() const {
  return m_aby_available_ports;
}

Port ConnectionHandler::initialize_aby_server(
    shared_ptr<RemoteConfiguration> remote_config) {
  string data{"{}"};
  list<string> headers{
    "Authorization: "s+ remote_config->get_remote_authenticator().sign_transaction(""),
    "Content-Type: application/json",
    };
  string url{assemble_remote_url(remote_config)+"/testConfig/"+ConfigurationHandler::cget().get_local_config()->get_local_id()};
  auto response{perform_post_request(url, data, headers, true)};
  // FIXME(TK): Auth from response
  auto resp_port(get_headers(response.body, "SEL-Port"));
  if(resp_port.empty()){
    throw runtime_error("No aby port for smpc communication in server response");
  }
  Port sel_port = stoul(resp_port.front());
  return sel_port;
}

Port ConnectionHandler::choose_aby_port() {
  lock_guard<mutex> lock(m_port_mutex);
  if (!m_aby_available_ports.empty()) {
    return move(m_aby_available_ports.extract(m_aby_available_ports.begin()).value());
  } else {
    throw runtime_error("No available port for smpc communication");
  }
}

void ConnectionHandler::mark_port_used(Port port) {
  if (auto it = m_aby_available_ports.find(port);
      it != m_aby_available_ports.end()) {
    lock_guard<mutex> lock(m_port_mutex);
    m_aby_available_ports.erase(it);
  } else {
    throw runtime_error("Can not mark port used: invalid port");
  }
}
void ConnectionHandler::populate_aby_ports() {
    m_aby_available_ports = ConfigurationHandler::cget().get_server_config().avaliable_aby_ports;
}
}  // namespace sel
