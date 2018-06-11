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
#include <memory>
#include <unordered_map>
#include "localconfiguration.h"
#include "remoteconfiguration.h"
#include "restbed"
#include "seltypes.h"

using namespace std;
namespace sel {
ConnectionHandler::ConnectionHandler(restbed::Service* service)
    : m_service(service) {}

void ConnectionHandler::upsert_connection(
    shared_ptr<RemoteConfiguration> connection) {
  RemoteId remote_id{connection->get_id()};
  if (connection_exists(remote_id)) {
    update_connection(remote_id, move(connection));
  } else {
    insert_connection(move(remote_id), move(connection));
  }
}

size_t ConnectionHandler::num_connections() const {
  return m_connections.size();
}

void ConnectionHandler::set_local_configuration(
    shared_ptr<LocalConfiguration>&& l_conf) {
  lock_guard<mutex> lock(m_local_data_mutex);
  m_local_configuration = move(l_conf);
}

bool ConnectionHandler::connection_exists(const RemoteId& c_id) const {
  auto it = m_connections.find(c_id);
  return (it != m_connections.end());
}

const ML_Field& ConnectionHandler::get_field(const FieldName& name) {
  return m_local_configuration->get_field(name);
}

shared_ptr<restbed::Service> ConnectionHandler::get_service() const {
  return m_service;
}

void ConnectionHandler::add_job(const RemoteId& remote_id,
                                shared_ptr<LinkageJob> job) {
  if (connection_exists(remote_id)) {
    auto job_id{job->get_id()};
    m_connections[remote_id]->add_job(move(job));
    m_job_id_to_remote_id.emplace(job_id, remote_id);
  } else {
    throw runtime_error("Invalid Remote ID");
  }
}

pair<unordered_map<JobId, shared_ptr<LinkageJob>>::iterator,
     unordered_map<JobId, shared_ptr<LinkageJob>>::iterator>
ConnectionHandler::find_job(const JobId& id) {
  if (auto it = m_job_id_to_remote_id.find(id);
      it != m_job_id_to_remote_id.end()) {
    auto& remote = m_connections[it->second];
    return remote->find_job(id);
  } else {
    throw runtime_error("Invalid Job ID");
  }
}

void ConnectionHandler::insert_connection(
    RemoteId&& remote_id,
    shared_ptr<RemoteConfiguration> connection) {
  m_connections.emplace(move(remote_id), move(connection));
}

void ConnectionHandler::update_connection(
    const JobId& remote_id,
    shared_ptr<RemoteConfiguration> connection) {
  m_connections[remote_id] = move(connection);
}

shared_ptr<LocalConfiguration> ConnectionHandler::get_local_configuration()
    const {
  return m_local_configuration;
}

shared_ptr<RemoteConfiguration> ConnectionHandler::get_remote_configuration(
    const RemoteId& r_id) {
  if (!connection_exists(r_id)) {
    throw runtime_error("Invalid Connection");
  }
  return m_connections.at(r_id);
}
}  // namespace sel
