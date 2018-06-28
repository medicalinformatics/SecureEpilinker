#include "datahandler.h"
#include "configurationhandler.h"
#include "databasefetcher.h"
#include "localconfiguration.h"
#include "seltypes.h"
#include <memory>
#include <mutex>

using namespace std;
namespace sel {

size_t DataHandler::poll_database() {
  const auto local_configuration{m_config_handler->get_local_config()};
  DatabaseFetcher database_fetcher{
      local_configuration, m_config_handler->get_algorithm_config(),
      local_configuration->get_data_service(),
      local_configuration->get_local_authentication()};
  database_fetcher.set_page_size(25u);  // TODO(TK): Magic number raus!
  auto data{database_fetcher.fetch_data()};
  lock_guard<mutex> lock(m_db_mutex);
  m_database = make_shared<const ServerData>(ServerData{
      move(data.hw_data), move(data.bin_data), move(data.hw_empty),
      move(data.bin_empty), move(data.ids), data.todate});
  // as hw and bin must have the same # of records, deduce nval from hw
  // not true, iff #hw fields == 0 || #bin fields == 0
  return (m_database->hw_data.begin()->second.size());
}

void DataHandler::set_config_handler(
    shared_ptr<ConfigurationHandler> conf_handler) {
  m_config_handler = conf_handler;
}

size_t DataHandler:: poll_database_diff() {
  return poll_database();
}

std::shared_ptr<const ServerData> DataHandler::get_database() const{
  lock_guard<mutex> lock(m_db_mutex);
  return m_database;
}
}  // namespace sel
