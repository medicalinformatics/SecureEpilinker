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
#include "util.h"

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
          {SERVER, m_config_handler->get_local_config()->get_aby_info().boolean_sharing,
           m_client_ip, m_client_port,
           m_config_handler->get_local_config()->get_aby_info().aby_threads},
          {m_config_handler->get_local_config()->get_weights(
               FieldComparator::NGRAM),
           m_config_handler->get_local_config()->get_weights(
               FieldComparator::BINARY),
           m_config_handler->get_local_config()->get_exchange_group_indices(
               FieldComparator::NGRAM),
           m_config_handler->get_local_config()->get_exchange_group_indices(
               FieldComparator::BINARY),
           m_config_handler->get_algorithm_config()->bloom_length,
           m_config_handler->get_algorithm_config()->threshold_match,
           m_config_handler->get_algorithm_config()->threshold_non_match}) {}
LocalServer::LocalServer(ClientId client_id,
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
  fmt::print("The Server is running and performing it's computations\n");
  const size_t nvals{m_data->hw_data.begin()->second.size()};
  vector<vector<Bitmask>> hw_data;
  vector<VCircUnit> bin_data;
  vector<vector<bool>> hw_empty;
  vector<vector<bool>> bin_empty;
  hw_data.reserve(m_data->hw_data.size());
  bin_data.reserve(m_data->bin_data.size());
  hw_empty.reserve(m_data->hw_empty.size());
  bin_empty.reserve(m_data->bin_empty.size());
  for (auto& field : m_data->hw_data){
    hw_data.emplace_back(field.second);
  }
  for (auto& field : m_data->bin_data){
    bin_data.emplace_back(field.second);
  }
  for (auto& field : m_data->hw_empty){
    hw_empty.emplace_back(field.second);
  }
  for (auto& field : m_data->bin_empty){
    bin_empty.emplace_back(field.second);
  }
  m_aby_server.build_circuit(nvals);
  m_aby_server.run_setup_phase();
  fmt::print("Starting Server\n");
  //auto server_result{m_aby_server.run_as_server({move(hw_data), move(bin_data), move(hw_empty), move(bin_empty)})};
  //
  //fmt::print("Server Result: {}", server_result);
  fmt::print("Returning dummy results\n");
  //return server_result;
  return {0,false,true
  #ifdef DEBUG_SEL_RESULT
    ,7,8
  #endif
  };
}

SecureEpilinker::Result LocalServer::launch_comparison(
    shared_ptr<const ServerData> data) {
  m_data = move(data);
  return run_server();
}
uint16_t LocalServer::get_port() const {
  return m_client_port;
}
}  // namespace sel
