/**
\file    monitormethodhandler.cpp
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
\brief Handles job status monitoring requests and returns status
*/

#include "monitormethodhandler.h"
#include <memory>
#include <string>
#include "connectionhandler.h"
#include "fmt/format.h"
#include "linkagejob.h"
#include "restbed"

using namespace std;
namespace sel {
void MonitorMethodHandler::handle_method(
    shared_ptr<restbed::Session> session) const {
  auto request{session->get_request()};
  auto headers{request->get_headers()};
  JobId job_id{request->get_path_parameter("job_id", "0")};
  fmt::print("Requested Job ID: {}\n", job_id);
  fmt::print("Recieved headers:\n");
  for (const auto& h : headers) {
    fmt::print("{} -- {}\n", h.first, h.second);
  }
  SessionResponse response;
  const auto id_iterators{m_connection_handler->find_job(job_id)};
  if (id_iterators.first != id_iterators.second) {  // Job ID found
    const auto status{
        js_enum_to_string(((*(id_iterators.first)).second)->get_status())};
    response.return_code = restbed::OK;
    response.body = status;
  } else {
    response.return_code = restbed::BAD_REQUEST;
    response.body = "Invalid job id";
  }
  response.headers = {{"Content-Length", to_string(response.body.length())},
                      {"Connection", "Close"}};
  session->close(response.return_code, response.body, response.headers);
}
}  // namespace sel
