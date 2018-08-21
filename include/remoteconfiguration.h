/**
\file    remoteconfiguration.h
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

#ifndef SEL_REMOTECONFIGURATION_H
#define SEL_REMOTECONFIGURATION_H
#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <map>
#include <vector>
#include "authenticationconfig.hpp"
#include "linkagejob.h"
#include "seltypes.h"
#include "resttypes.h"

namespace sel {
class MatchingJob;
class ServerHandler;
class ConnectionHandler;

class RemoteConfiguration {
  /**
   * Stores secureEpiLinker configuration for one connection
   */
 public:
  explicit RemoteConfiguration(RemoteId c_id);
  ~RemoteConfiguration();
  void set_connection_profile(ConnectionConfig cconfig);

  void set_linkage_service(ConnectionConfig cconfig);
  ConnectionConfig const * get_linkage_service() const;

  RemoteId get_id() const;

  Port get_remote_signaling_port() const;
  Port get_aby_port() const;
  void set_aby_port(Port port);
  std::string get_remote_host() const;
  std::string get_remote_scheme() const;

  void set_matching_mode(bool);
  bool get_matching_mode() const;

  bool get_mutual_initialization_status() const;

  void test_configuration(const RemoteId&, const nlohmann::json&);
  void mark_mutually_initialized() const; // changes mutable flag
 protected:
 private:
  RemoteId m_connection_id;
  ConnectionConfig m_connection_profile;
  ConnectionConfig m_linkage_service;
  Port m_aby_port;
  bool m_matching_mode{false};
  mutable bool m_mutually_initialized{false};
};

}  // Namespace sel

#endif  // SEL_REMOTECONFIGURATION_H
