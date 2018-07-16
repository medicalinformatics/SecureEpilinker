/**
\file    serverhandler.h
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
\brief Holds and handles ABY server and clients
*/

#ifndef SEL_SERVERHANDLER_H
#define SEL_SERVERHANDLER_H
#pragma once

#include "seltypes.h"
#include "resttypes.h"
#include "secure_epilinker.h"
#include "connectionhandler.h"
#include <map>
#include <memory>

namespace spdlog{
  class logger;
}

namespace sel {
class LinkageJob;
class LocalServer;
class ConfigurationHandler;
class DataHandler;

class ServerHandler {
  public:
    ServerHandler(std::shared_ptr<ConfigurationHandler>, std::shared_ptr<DataHandler>);
    void insert_client(RemoteId);
    void insert_server(ClientId, RemoteAddress);
    void add_linkage_job(const RemoteId&, std::shared_ptr<LinkageJob>&&);
    std::shared_ptr<const LinkageJob> get_linkage_job(const JobId&) const;
    std::shared_ptr<LocalServer> get_local_server(const ClientId&) const;
    uint16_t get_server_port(const ClientId&) const;
    std::shared_ptr<SecureEpilinker> get_epilink_client(const RemoteId&);
    void run_server(ClientId, std::shared_ptr<const ServerData>);
  private:
    std::map<JobId, RemoteId> m_job_remote_mapping;
    std::map<RemoteId, std::shared_ptr<SecureEpilinker>> m_aby_clients;
    std::map<RemoteId, std::map<JobId, std::shared_ptr<LinkageJob>> > m_client_jobs;
    std::map<ClientId, std::shared_ptr<LocalServer>> m_server;
    std::shared_ptr<ConfigurationHandler> m_config_handler;
    std::shared_ptr<DataHandler> m_data_handler;
    std::shared_ptr<spdlog::logger> m_logger;
};

} // namespace sel


#endif /* end of include guard: SEL_SERVERHANDLER_H */
