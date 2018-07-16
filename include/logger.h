/**
\file    logger.h
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
#ifndef SEL_LOGGER_H
#define SEL_LOGGER_H
#pragma once

#include <string>
#include <memory>
#include <spdlog/spdlog.h>

namespace sel{
constexpr char logger_name[]{"default"};
constexpr size_t async_log_queue_size{8192u};
constexpr size_t log_file_size{1024u*1024u*5u};
constexpr unsigned log_history{5u};
constexpr unsigned logging_threads{1u};

void createLogger(const std::string& filename, spdlog::level::level_enum level);
inline std::shared_ptr<spdlog::logger> get_default_logger(){
  return spdlog::get(logger_name);
}

} // namespace sel
#endif /* end of include guard: SEL_LOGGER_H */
