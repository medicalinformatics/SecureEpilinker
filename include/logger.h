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

namespace sel {

 /*! \enum ComponentLogger
  *
  *  Enumerates all logger options for subcomponents
  */
enum class ComponentLogger {MAIN, CIRCUIT, CLEARCIRCUIT, TEST, REST, SERVER, CLIENT};

/**
 * Create a rotating file + stdout logger
 */
void create_file_logger(const std::string& filename);

/**
 * Create a colored stdout logger
 */
void create_terminal_logger();

/**
 * Returns the default logger, which got created by any create*() function
 */
std::shared_ptr<spdlog::logger> get_logger(ComponentLogger = ComponentLogger::MAIN);

} // namespace sel

#endif /* end of include guard: SEL_LOGGER_H */
