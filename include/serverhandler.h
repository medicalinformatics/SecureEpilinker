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
#include "connectionhandler.h"
#include "serialworker.hpp"
#include "logger.h"
#include <map>
#include <memory>

namespace sel {

class LinkageJob;
class LocalServer;
class ConfigurationHandler;
class DataHandler;
class SecureEpilinker;

class ServerHandler {
  public:
    static ServerHandler& get();
    static ServerHandler const& cget();
    void insert_client(RemoteId);
    void insert_server(RemoteId, RemoteAddress);
    void add_linkage_job(const RemoteId&, const std::shared_ptr<LinkageJob>&);
    std::shared_ptr<const LinkageJob> get_linkage_job(const JobId&) const;
    std::string get_job_status(const JobId&) const;
    std::shared_ptr<LocalServer> get_local_server(const RemoteId&) const;
    Port get_server_port(const RemoteId&) const;
    std::shared_ptr<SecureEpilinker> get_epilink_client(const RemoteId&);
    void run_server(const RemoteId&, std::shared_ptr<const ServerData>, size_t, bool);
    void connect_client(const RemoteId&);
  protected:
    ServerHandler() = default;
  private:
    ~ServerHandler();
    std::map<RemoteId, std::shared_ptr<SecureEpilinker>> m_aby_clients;
    std::map<RemoteId, std::shared_ptr<LocalServer>> m_server;
    std::map<RemoteId, SerialWorker<LinkageJob>> m_worker_threads;
    std::map<JobId, std::shared_ptr<LinkageJob>> m_client_jobs; // for status retrieval
    std::shared_ptr<spdlog::logger> m_logger{get_logger(ComponentLogger::SERVER)};
};

} // namespace sel


#endif /* end of include guard: SEL_SERVERHANDLER_H */
