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

namespace sel {
class MatchingJob;

class RemoteConfiguration {
  /**
   * Stores secureEpiLinker configuration for one connection
   */
 public:
  explicit RemoteConfiguration(RemoteId c_id);
  ~RemoteConfiguration();
  void set_connection_profile(ConnectionConfig cconfig);

  void set_linkage_service(ConnectionConfig cconfig);

  RemoteId get_id() const;
  void set_remote_client_id(std::string id){m_remote_client_id = move(id);}
  ClientId get_remote_client_id() const  {return m_remote_client_id;}

  uint16_t get_remote_signaling_port() const;
  uint16_t get_aby_port() const;
  void set_aby_port(uint16_t port);
  std::string get_remote_host() const;

 protected:
 private:
  RemoteId m_connection_id;
  ClientId m_remote_client_id;
  ConnectionConfig m_connection_profile;
  ConnectionConfig m_linkage_service;
  uint16_t m_aby_port;
};

}  // Namespace sel

#endif  // SEL_REMOTECONFIGURATION_H
