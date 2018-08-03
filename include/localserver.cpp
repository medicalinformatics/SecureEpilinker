/**
\file    localserver.h
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

LocalServer::LocalServer(ClientId client_id,
                         std::string client_ip,
                         uint16_t client_port,
                         shared_ptr<DataHandler> data_handler,
                         shared_ptr<ConfigurationHandler> config_handler)
    : m_client_id(move(client_id)),
      m_client_ip(move(client_ip)),
      m_client_port(client_port),
      m_data_handler(move(data_handler)),
      m_config_handler(move(config_handler)),
      m_aby_server(
          {SERVER, m_config_handler->get_server_config().boolean_sharing,
           m_client_ip, m_client_port,
           m_config_handler->get_server_config().aby_threads},
          {m_config_handler->get_local_config()->get_fields(),
           m_config_handler->get_local_config()->get_exchange_groups(),
           m_config_handler->get_algorithm_config()->threshold_match,
LocalServer::LocalServer(ClientId client_id,
           m_config_handler->get_algorithm_config()->threshold_non_match,
           m_config_handler->get_remote_config(remote_id)->get_matching_mode()}) {}
                         SecureEpilinker::ABYConfig aby_config,
                         EpilinkConfig epi_config,
                         shared_ptr<DataHandler> data_handler,
                         shared_ptr<ConfigurationHandler> config_handler)
    : m_client_id(move(client_id)),
      m_client_ip(aby_config.remote_host),
      m_client_port(aby_config.port),
      m_data_handler(move(data_handler)),
      m_config_handler(move(config_handler)),
      m_aby_server( aby_config, epi_config) {}

ClientId LocalServer::get_id() const {
  return m_client_id;
}

SecureEpilinker::Result LocalServer::run_server() {
  auto logger{get_default_logger()};
  logger->info("The Server is running and performing it's computations");
  const size_t nvals{m_data->data.begin()->second.size()};
  m_aby_server.build_circuit(nvals);
  m_aby_server.run_setup_phase();
  logger->debug("Starting Server Computation");
  EpilinkServerInput data{{m_data->data}};
  auto server_result{m_aby_server.run_as_server({m_data->data})};
  m_aby_server.reset();
#ifdef DEBUG_SEL_REST
  auto debugger{m_data_handler->get_epilink_debug()};
  debugger->server_input = std::make_shared<EpilinkServerInput>(data);
#endif
  return server_result;
  }

SecureEpilinker::Result LocalServer::launch_comparison(
    shared_ptr<const ServerData> data) {
  m_data = move(data);
  return run_server();
}
uint16_t LocalServer::get_port() const {
  return m_client_port;
}

string LocalServer::get_ip() const {
  return m_client_ip;
}

bool LocalServer::compare_configuration(const nlohmann::json& client_config) const {
  nlohmann::json server_config{*(m_config_handler->get_algorithm_config())};
  server_config.emplace_back(m_config_handler->get_local_config()->get_comparison_json());

  return client_config == server_config;
}
}  // namespace sel
