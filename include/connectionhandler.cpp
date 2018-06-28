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
#include "restbed"
#include "seltypes.h"
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Infos.hpp>
#include <sstream>

using namespace std;
namespace sel {
ConnectionHandler::ConnectionHandler(restbed::Service* service)
    : m_service(service) {}

shared_ptr<restbed::Service> ConnectionHandler::get_service() const {
  return m_service;
}

uint16_t ConnectionHandler::get_free_port() {
  if (auto it = m_aby_available_ports.begin(); !m_aby_available_ports.empty()) {
    auto port = *it;
    m_aby_available_ports.erase(it);
    return port;
  } else {
    throw std::runtime_error("No remaining port for ABY");
  }
  return 0u;
}

ConnectionHandler::RemoteInfo ConnectionHandler::initialize_aby_server(
    shared_ptr<RemoteConfiguration> remote_config) {
  if (m_aby_available_ports.empty()) {
    throw std::runtime_error("No available port for smpc communication");
  }
  auto ip{remote_config->get_remote_host()};
  auto port{remote_config->get_remote_signaling_port()};
  auto id{generate_id()};
  curlpp::Easy curl_request;
  stringstream response_stream;
  list<string> headers{
    "Authorization: SEL ABCD",
    "Available-Ports: "s+get_available_ports(),
    "Remote-Identifier: "s+id,
    "SEL-Init: True"};
  curl_request.setOpt(new curlpp::Options::HttpHeader(headers));
  curl_request.setOpt(new curlpp::Options::Url("https://"s+ip+':'+to_string(port)+"/selconnect"));
  curl_request.setOpt(new curlpp::Options::Post(true));
  curl_request.setOpt(new curlpp::Options::SslVerifyHost(false));
  curl_request.setOpt(new curlpp::Options::SslVerifyPeer(false));
  curl_request.setOpt(new curlpp::Options::WriteStream(&response_stream));
  curl_request.setOpt(new curlpp::options::Header(1));
  curl_request.perform();
  auto resph(get_headers(response_stream, "SEL-Port"));
  if(resph.empty()){
    throw runtime_error("No common available port for smpc communication");
  }
  uint16_t sel_port = stoul(resph.front());
  return {id, sel_port};
}

uint16_t ConnectionHandler::choose_common_port(const string& remote_ports) {
  set<uint16_t> remote_ports_set;
  auto remote_ports_vec{split(remote_ports, ',')};  // Expects ports as csv
  for (const auto& port : remote_ports_vec) {
    remote_ports_set.emplace(stoul(port));
  }
  vector<uint16_t> intersection;
  lock_guard<mutex> lock(m_port_mutex);
  set_intersection(m_aby_available_ports.begin(), m_aby_available_ports.end(),
                   remote_ports_set.begin(), remote_ports_set.end(),
                   back_inserter(intersection));
  if (intersection.empty()) {
    throw runtime_error("No common available port for smpc communication");
  }
  m_aby_available_ports.erase(intersection.front());
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
void ConnectionHandler::mark_port_used(uint16_t port) {
  if (auto it = m_aby_available_ports.find(port);
      it != m_aby_available_ports.end()) {
    lock_guard<mutex> lock(m_port_mutex);
    m_aby_available_ports.erase(it);
  } else {
    throw runtime_error("Can not mark port used: invalid port");
  }
}
}  // namespace sel
