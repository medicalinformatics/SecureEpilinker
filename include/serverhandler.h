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
#include <map>
#include <memory>
#include "logger.h"

namespace sel {

class LinkageJob;
class LocalServer;
class ConfigurationHandler;
class DataHandler;
class SecureEpilinker;

class ServerHandler {
  protected:
    ServerHandler(size_t);
  public:
    static ServerHandler& get();
    static ServerHandler const& cget();
    void insert_client(RemoteId);
    void insert_server(RemoteId, RemoteAddress);
    void add_linkage_job(const RemoteId&, std::shared_ptr<LinkageJob>&&);
    std::shared_ptr<const LinkageJob> get_linkage_job(const JobId&) const;
    std::string get_job_status(const JobId&) const;
    std::shared_ptr<LocalServer> get_local_server(const RemoteId&) const;
    Port get_server_port(const RemoteId&) const;
    std::shared_ptr<SecureEpilinker> get_epilink_client(const RemoteId&);
    void run_server(RemoteId, std::shared_ptr<const ServerData>);
    void connect_client(const RemoteId&);
  private:
    void run_job(std::shared_ptr<LinkageJob>&);
    std::shared_ptr<LinkageJob> retrieve_next_queued_job(size_t remote_counter);
    void execute_job_queue(size_t id);
    std::map<JobId, RemoteId> m_job_remote_mapping;
    std::map<RemoteId, std::shared_ptr<SecureEpilinker>> m_aby_clients;
    std::map<RemoteId, std::map<JobId, std::shared_ptr<LinkageJob>> > m_client_jobs;
    std::map<RemoteId, std::shared_ptr<LocalServer>> m_server;
    std::shared_ptr<spdlog::logger> m_logger{get_default_logger()};
    std::mutex m_job_queue_mutex;
    std::vector<std::thread> m_worker_threads;
};

} // namespace sel


#endif /* end of include guard: SEL_SERVERHANDLER_H */
