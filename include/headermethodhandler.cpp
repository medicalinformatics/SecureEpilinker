/**
\file    headermethodhandler.cpp
\author  Tobias Kussel <kussel@cbs.tu-darmstadt.de>
\copyright SEL - Secure EpiLinker
    Copyright (C) 2018 Computational Biology & Simulation Group TU-Darmstadt
This program is free software: you can redistribute it and/or modify it under
the terms of the GNU Affero General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option) any
later version. This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Affero General Public License for more details.
    You should have received a copy of the GNU Affero General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.  
\brief handles requests without data payload but relevant headers 
*/

#include "headermethodhandler.h"
#include <memory>
#include <string>
#include "configurationhandler.h"
#include "connectionhandler.h"
#include "serverhandler.h"
#include "datahandler.h"
#include "methodhandler.hpp"
#include "restbed"
#include "util.h"
#include "resttypes.h"
#include "logger.h"

using namespace std;
namespace sel {
HeaderMethodHandler::HeaderMethodHandler(
      const string& method,
      function<SessionResponse(
        const std::shared_ptr<restbed::Session>&,
        const std::shared_ptr<const restbed::Request>&,
        const multimap<string,string>&,
        const string&,
        const shared_ptr<spdlog::logger>&)> handling_function)
      : MethodHandler(method),
        m_logger(get_default_logger()),
        m_handling_function(move(handling_function)) {}

void HeaderMethodHandler::set_handling_function(function<SessionResponse(
                                  const shared_ptr<restbed::Session>&, 
                                  const std::shared_ptr<const restbed::Request>&,
                                  const multimap<string,string>&,
                                  const string&,
                                  const shared_ptr<spdlog::logger>&)> fun){
  m_handling_function = move(fun);
}

void HeaderMethodHandler::handle_method(
    shared_ptr<restbed::Session> session) const {
  auto request{session->get_request()};
  auto headers{request->get_headers()};
  string parameter{request->get_path_parameter("parameter", "")};
  string header_string;
  for (const auto& h : headers) {
    header_string += h.first + " -- " + h.second + "\n";
  }
  m_logger->debug("HeaderHandler used\nParameter: {}\nRecieved headers\n{}", parameter, header_string);
  SessionResponse response;
  if(m_handling_function){
    response = m_handling_function(session, request, headers, parameter, m_logger);
  } else {
    throw runtime_error("Invalid handling function");
  }
  session->close(response.return_code, response.body, response.headers);
}
}  // namespace sel
