#include "datahandler.h"
#include "configurationhandler.h"
#include "databasefetcher.h"
#include "localconfiguration.h"
#include "remoteconfiguration.h"
#include "clear_epilinker.h"
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>

using namespace std;
namespace sel {

#ifdef DEBUG_SEL_REST
bool Debugger::all_values_set() const{
  return client_input && server_input && circuit_config;
}

void Debugger::compute_int() {
  int_result = clear_epilink::calc<CircUnit>(client_input.value(), server_input.value(),circuit_config.value());
}

void Debugger::compute_double() {
  double_result = clear_epilink::calc<double>(client_input.value(), server_input.value(),circuit_config.value());
}

void Debugger::reset() {
  client_input.reset(); server_input.reset(); circuit_config.reset();
  int_result = {}; double_result = {};
  run = false;
}
#endif

  DataHandler& DataHandler::get() {
    static DataHandler singleton;
    return singleton;
  }

  DataHandler const& DataHandler::cget() {
    return cref(get());
  }

size_t DataHandler::poll_database(const RemoteId& remote_id, bool counting_mode) {
  const auto& config_handler{ConfigurationHandler::cget()};
  const auto local_configuration{config_handler.get_local_config()};
  DatabaseFetcher database_fetcher{
      local_configuration,
      local_configuration->get_data_service()+"/"+remote_id,
      local_configuration->get_local_authenticator(),
      config_handler.get_server_config().default_page_size};
  auto data{database_fetcher.fetch_data(counting_mode)};
  lock_guard<mutex> lock(m_db_mutex);
  m_database = make_shared<const ServerData>(move(data));
  return (m_database->data->begin()->second.size());
}

size_t DataHandler:: poll_database_diff() {
  // TODO(TK): Implement
  //return poll_database();
  return 0;
}

std::shared_ptr<const ServerData> DataHandler::get_database() const{
  lock_guard<mutex> lock(m_db_mutex);
  return m_database;
}
}  // namespace sel
