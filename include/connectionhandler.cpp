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
#include "seltypes.h"
#include "localconfiguration.h"
#include "remoteconfiguration.h"
#include "restbed"
#include <memory>
#include <unordered_map>

sel::ConnectionHandler::ConnectionHandler(restbed::Service* service)
    : m_service(service) {}

void sel::ConnectionHandler::upsert_connection(
    std::shared_ptr<sel::RemoteConfiguration> connection) {
  sel::RemoteId remote_id{connection->get_id()};
  if (connection_exists(remote_id)) {
    update_connection(remote_id, std::move(connection));
  } else {
    insert_connection(std::move(remote_id), std::move(connection));
  }
}

size_t sel::ConnectionHandler::num_connections() const {
  return m_connections.size();
}

void sel::ConnectionHandler::set_local_configuration(
    std::unique_ptr<LocalConfiguration>&& l_conf) {
  m_local_configuration = std::move(l_conf);
}

bool sel::ConnectionHandler::connection_exists(const sel::RemoteId& c_id) const {
  auto it = m_connections.find(c_id); 
   return (it != m_connections.end());
}

const sel::ML_Field& sel::ConnectionHandler::get_field(
    const sel::FieldName& name) {
  return m_local_configuration->get_field(name);
}

std::shared_ptr<restbed::Service> sel::ConnectionHandler::get_service() const {
  return m_service;
}

void sel::ConnectionHandler::add_job(const sel::RemoteId& remote_id,
                                     std::shared_ptr<sel::LinkageJob> job) {
  if (connection_exists(remote_id)) {
    auto job_id{job->get_id()};
    m_connections[remote_id]->add_job(std::move(job));
    m_job_id_to_remote_id.emplace(job_id, remote_id);
  } else {
    throw std::runtime_error("Invalid Remote ID");
  }
}

std::pair<
    std::unordered_map<sel::JobId, std::shared_ptr<sel::LinkageJob>>::iterator,
    std::unordered_map<sel::JobId, std::shared_ptr<sel::LinkageJob>>::iterator>
sel::ConnectionHandler::find_job(const sel::JobId& id) {
  if (auto it = m_job_id_to_remote_id.find(id);
      it != m_job_id_to_remote_id.end()) {
    auto& remote = m_connections[it->second];
    return remote->find_job(id);
  } else {
    throw std::runtime_error("Invalid Job ID");
  }
}

void sel::ConnectionHandler::insert_connection(
    sel::RemoteId&& remote_id,
    std::shared_ptr<sel::RemoteConfiguration> connection) {
  m_connections.emplace(std::move(remote_id), std::move(connection));
}

void sel::ConnectionHandler::update_connection(
    const sel::JobId& remote_id,
    std::shared_ptr<sel::RemoteConfiguration> connection) {
  m_connections[remote_id] = std::move(connection);
}
