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

constexpr char logger_name[]{"Main"};
constexpr size_t async_log_queue_size{8192u};
constexpr size_t log_file_size{1024u*1024u*5u};
constexpr unsigned log_history{5u};
constexpr unsigned logging_threads{3u};

void setup_default_logger() {
  spdlog::flush_on(spdlog::level::debug);
  spdlog::flush_every(30s);
  spdlog::init_thread_pool(async_log_queue_size,logging_threads);
}

void register_multisink_logger(const vector<spdlog::sink_ptr>& sinks) {
  auto multisink = make_shared<spdlog::logger>(logger_name, sinks.begin(), sinks.end());
  spdlog::register_logger(multisink);
}

void create_file_logger(const std::string& filename) {
  setup_default_logger();

  vector<spdlog::sink_ptr> sinks = {
    make_shared<spdlog::sinks::stdout_color_sink_mt>(),
    make_shared<spdlog::sinks::rotating_file_sink_mt>(filename,log_file_size,log_history)
  };

  register_multisink_logger(sinks);
  spdlog::set_pattern("[%Y-%m-%d %T.%e][%n][%t][%^%l%$] %v");
}

void create_terminal_logger() {
  setup_default_logger();

  vector<spdlog::sink_ptr> sinks = {
    make_shared<spdlog::sinks::stdout_color_sink_mt>()
  };

  register_multisink_logger(sinks);
  spdlog::set_pattern("[%Y-%m-%d %T.%e][%n][%t]%^[%l]%$ %v");
}

std::shared_ptr<spdlog::logger> get_logger(ComponentLogger subcomponent){
  auto rootlogger{spdlog::get(logger_name)};
  switch (subcomponent) {
    case ComponentLogger::MAIN: return rootlogger;
    case ComponentLogger::CIRCUIT: return rootlogger->clone("Circuit");
    case ComponentLogger::CLEARCIRCUIT: return rootlogger->clone("ClearCircuit");
    case ComponentLogger::TEST: return rootlogger->clone("Test");
    case ComponentLogger::REST: return rootlogger->clone("REST");
    case ComponentLogger::SERVER: return rootlogger->clone("Server");
    case ComponentLogger::CLIENT: return rootlogger->clone("Client");
    default: assert(!"You forgot an enum case in logger.cpp"); return rootlogger;
  }
}

} /* END namespace sel */
