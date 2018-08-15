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
ConnectionHandler::ConnectionHandler(restbed::Service* service)
    : m_service(service) {}

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
  if (m_aby_available_ports.empty()) {
    throw std::runtime_error("No available port for smpc communication");
  }
  string data{"{}"};
  list<string> headers{
    "Authorization: SEL ABCD", //FIXME(TK) Need to get AUTH
    "Available-Ports: "s+get_available_ports(),
    "Content-Type: application/json",
    };
  string url{assemble_remote_url(remote_config)+"/testConfig/"+m_config_handler->get_local_config()->get_local_id()};
  auto response{perform_post_request(url, data, headers, true)};
  auto resp_port(get_headers(response.body, "SEL-Port"));
  if(resp_port.empty()){
    throw runtime_error("No common available port for smpc communication");
  }
  Port sel_port = stoul(resp_port.front());
  return sel_port;
}

Port ConnectionHandler::choose_common_port(const set<Port>& remote_ports) {
  vector<Port> intersection;
  lock_guard<mutex> lock(m_port_mutex);
  set_intersection(m_aby_available_ports.begin(), m_aby_available_ports.end(),
                   remote_ports.begin(), remote_ports.end(),
                   back_inserter(intersection));

  if (intersection.empty()) {
    throw runtime_error("No common available port for smpc communication");
  }
  m_aby_available_ports.erase(m_aby_available_ports.find(intersection.front()));
  return intersection.front();
}

string ConnectionHandler::get_available_ports() const {
  string result;
  for (const auto& port : m_aby_available_ports) {
    result += to_string(port) + ',';
  }
  result.pop_back();  // remove trailing comma
  return result;
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
  if(m_config_handler){
    m_aby_available_ports = m_config_handler->get_server_config().avaliable_aby_ports;
  }
}
}  // namespace sel
