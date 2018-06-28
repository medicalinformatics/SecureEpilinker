/**
\file    remoteconfiguration.cpp
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
\brief Holds Information about a remote connection
*/

#include "remoteconfiguration.h"
#include <mutex>
#include "seltypes.h"
#include "util.h"

using namespace std;
namespace sel {

RemoteConfiguration::RemoteConfiguration(RemoteId c_id)
    : m_connection_id(move(c_id)) {}

RemoteConfiguration::~RemoteConfiguration() {}

uint16_t RemoteConfiguration::get_remote_signaling_port() const {
  auto parts{split(m_connection_profile.url, ':')};
  // {{192.168.1.1},{8080}}
  return stoi(parts.back());
}

string RemoteConfiguration::get_remote_host() const {
  auto parts{split(m_connection_profile.url, ':')};
  // {{192.168.1.1},{8080}}
  return parts.front();
}

void RemoteConfiguration::set_connection_profile(ConnectionConfig cconfig) {
  m_connection_profile = std::move(cconfig);
}
void RemoteConfiguration::set_linkage_service(ConnectionConfig cconfig) {
  m_linkage_service = std::move(cconfig);
}

RemoteId RemoteConfiguration::get_id() const {
  return m_connection_id;
}

uint16_t RemoteConfiguration::get_aby_port() const {
  return m_aby_port;
}

void RemoteConfiguration::set_aby_port(uint16_t port) {
  m_aby_port = port;
}

}  // namespace sel
