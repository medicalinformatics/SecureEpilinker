/**
\file    tempselmethodhandler.cpp
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
\brief starts Database polling
*/

#include "tempselmethodhandler.h"
#include "connectionhandler.h"
#include "linkagejob.h"
#include "fmt/format.h"
#include <memory>
#include <string>
#include "restbed"


void sel::TempSelMethodHandler::handle_method(
    std::shared_ptr<restbed::Session> session) const {
  auto request{session->get_request()};
  auto headers{request->get_headers()};
  fmt::print("Recieved Job Request\n");

  SessionResponse response;
  m_connection_handler->initiate_comparison();
    response.return_code = restbed::OK;
    std::string status{"Calculating Stuff"};
    response.body = status;
  response.headers = {{"Content-Length", std::to_string(response.body.length())},
                                                        {"Connection", "Close"}};
  session->close(response.return_code, response.body, response.headers);
}
