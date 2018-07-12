/**
\file    tempselmethodhandler.cpp
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
\brief starts Database polling
*/

#include "tempselmethodhandler.h"
#include <memory>
#include <string>
#include "connectionhandler.h"
#include "datahandler.h"
#include "fmt/format.h"
#include "linkagejob.h"
#include "localconfiguration.h"
#include "restbed"
#include "serverhandler.h"
#include "util.h"
#include "resttypes.h"
#include <thread>

using namespace std;
namespace sel {
void TempSelMethodHandler::handle_method(
    shared_ptr<restbed::Session> session) const {
  auto request{session->get_request()};
  auto headers{request->get_headers()};
  SessionResponse response;
  uint16_t common_port;
  string client_ip;
  ClientId client_id;
    fmt::print("Recieved Server Initialization Request\n");
    // TODO(TK) Authorize Usage
    if (auto it = headers.find("Available-Ports"); it != headers.end()) {
      common_port = m_connection_handler->choose_common_port(it->second);
      const auto origin{session->get_origin()};
      const bool ipv6{split(origin,':').size() > 2 ? true : false};
      fmt::print("Origin: {}, is IPv{}\n", origin, ipv6?"6":"4");
      if(!ipv6){
        client_ip = split(origin, ':').front();
      } else {
        client_ip = "127.0.0.1"; // FIXME(TK): Handling for IPv6
      }
      client_id = request->get_header("Remote-Identifier", "Not set");
      response.return_code = restbed::OK;
      response.body = "Create Server";
      response.headers = {{"Content-Length", to_string(response.body.length())},
                          {"SEL-Port", to_string(common_port)},
                          {"Connection", "Close"}};
      session->close(response.return_code, response.body, response.headers);

      fmt::print("Response created\n");
      auto fun = bind(&ServerHandler::insert_server,m_server_handler.get(),client_id,placeholders::_1);
      RemoteAddress tempadr{client_ip,common_port};
      std::thread server_creator(fun, tempadr);
      server_creator.detach();
    }
}
}  // namespace sel
