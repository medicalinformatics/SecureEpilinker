/**
\file    localserver.cpp
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
\brief Performs ABY server computation
*/

#include "localserver.h"
#include <string>
#include "configurationhandler.h"
#include "remoteconfiguration.h"
#include "datahandler.h"
#include "fmt/format.h"
#include "localconfiguration.h"
#include "secure_epilinker.h"
#include "seltypes.h"
#include "resttypes.h"
#include "util.h"
#include "logger.h"

using namespace std;
namespace sel {

LocalServer::LocalServer(RemoteId remote_id,
                         std::string client_ip,
                         Port client_port)
    : m_remote_id(move(remote_id)),
      m_client_ip(move(client_ip)),
      m_client_port(client_port),
      m_aby_server(
          {SERVER,
           static_cast<e_sharing>(ConfigurationHandler::cget().get_server_config().boolean_sharing),
           m_client_ip, m_client_port,
           ConfigurationHandler::cget().get_server_config().aby_threads},
          make_circuit_config(ConfigurationHandler::cget().get_local_config(),
            ConfigurationHandler::cget().get_remote_config(remote_id))) {}

LocalServer::LocalServer(RemoteId remote_id,
                         SecureEpilinker::ABYConfig aby_config,
                         CircuitConfig circuit_config)
    : m_remote_id(move(remote_id)),
      m_client_ip(aby_config.host),
      m_client_port(aby_config.port),
      m_aby_server(aby_config, circuit_config) {}

RemoteId LocalServer::get_id() const {
  return m_remote_id;
}

vector<Result<CircUnit>> LocalServer::run_linkage(shared_ptr<const ServerData> data, size_t num_records) {
  m_data = move(data);
  auto logger{get_logger()};
  logger->info("The server is running and performing its linkage computations");
  const size_t database_size{m_data->data->begin()->second.size()};
#ifdef DEBUG_SEL_REST
  auto debugger{DataHandler::get().get_epilink_debug()};
  logger->warn("Setting Server Debug inputs");
  debugger->server_input = *(m_data->data);
#endif
  logger->debug("Server Sizes\nnum_records: {}, database_size: {}", num_records, database_size);
    m_aby_server.build_linkage_circuit(num_records, database_size);
    m_aby_server.run_setup_phase();
    logger->debug("Starting server linkage computation");
    m_aby_server.set_server_input({m_data->data, num_records});
    auto linkage_result = m_aby_server.run_linkage();
  m_aby_server.reset();
  return linkage_result;
}

CountResult<CircUnit> LocalServer::run_count(shared_ptr<const ServerData> data, size_t num_records) {
  m_data = move(data);

  auto logger{get_logger()};
  logger->info("The server is running and performing its matching computations");

  const size_t database_size{m_data->data->begin()->second.size()};
    m_aby_server.build_count_circuit(num_records, database_size);
    m_aby_server.run_setup_phase();
    logger->debug("Starting server matching computation");
    m_aby_server.set_input({m_data->data, num_records});
    auto count_result = m_aby_server.run_count();
  m_aby_server.reset();

  return count_result;
}

Port LocalServer::get_port() const {
  return m_client_port;
}

string LocalServer::get_ip() const {
  return m_client_ip;
}

SecureEpilinker& LocalServer::get_epilinker() {
  return m_aby_server;
}

void LocalServer::connect_server() {
  m_aby_server.connect();
}
}  // namespace sel
