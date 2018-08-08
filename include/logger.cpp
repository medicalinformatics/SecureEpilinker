/**
\file    logger.cpp
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
\brief Configuration of logging instances
*/

#include "logger.h"
#include <memory>
#include <vector>
#include <string>
#include <spdlog/async.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/dist_sink.h>

using namespace std;
namespace sel{
void createLogger(const std::string& filename){
  spdlog::flush_on(spdlog::level::debug);
  spdlog::flush_every(30s);
  spdlog::init_thread_pool(async_log_queue_size,logging_threads);
  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
  sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(filename,log_file_size,log_history));
  //for(auto& sink : sinks) {
    //sink->set_level(level);
  //}

  auto multisink = std::make_shared<spdlog::logger>(logger_name, sinks.begin(), sinks.end());
  spdlog::register_logger(multisink);
}
} // namespace sel
