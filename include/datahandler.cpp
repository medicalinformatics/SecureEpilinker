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
  int_result = clear_epilink::calc_integer({*client_input, *server_input},*circuit_config);
}

void Debugger::compute_double() {
  double_result = clear_epilink::calc_exact({*client_input, *server_input},*circuit_config);
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

size_t DataHandler::poll_database(const RemoteId& remote_id) {
  const auto& config_handler{ConfigurationHandler::cget()};
  const auto local_configuration{config_handler.get_local_config()};
  DatabaseFetcher database_fetcher{
      local_configuration,
      local_configuration->get_data_service()+"/"+remote_id,
      local_configuration->get_local_authentication(),
      config_handler.get_server_config().default_page_size};
  auto data{database_fetcher.fetch_data(config_handler.get_remote_config(remote_id)->get_matching_mode())};
  lock_guard<mutex> lock(m_db_mutex);
  m_database = make_shared<const ServerData>(move(data));
  return (m_database->data->begin()->second.size());
}

size_t DataHandler:: poll_database_diff() {
  // TODO(TK): Implement
  //return poll_database();
  return 0;
}

unique_ptr<const VRecord> DataHandler::get_client_records(const nlohmann::json& client_data) const {
    DatabaseFetcher db_fetcher(ConfigurationHandler::cget().get_local_config());
    db_fetcher.save_page_data(client_data, false, false);
    return move(db_fetcher.move_client_data());
}

std::shared_ptr<const ServerData> DataHandler::get_database() const{
  lock_guard<mutex> lock(m_db_mutex);
  return m_database;
}
}  // namespace sel
