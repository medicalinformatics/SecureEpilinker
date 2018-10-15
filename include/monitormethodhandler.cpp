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
#include "fmt/format.h"
#include "linkagejob.h"
#include "logger.h"
#include "restbed"
#include "resttypes.h"
#include "serverhandler.h"

using namespace std;
namespace sel {
MonitorMethodHandler::MonitorMethodHandler(
    const std::string& method)
    : MethodHandler(method),
      m_logger{get_logger()} {}

MonitorMethodHandler::MonitorMethodHandler(
    const std::string& method,
    std::shared_ptr<Validator>& validator)
    : MethodHandler(method, validator),
      m_logger{get_logger()} {}

void MonitorMethodHandler::handle_method(
    shared_ptr<restbed::Session> session) const {
  auto request{session->get_request()};
  auto headers{request->get_headers()};
  JobId job_id{request->get_path_parameter("job_id", "list")};
  if(job_id == "list") {
    m_logger->info("Requested status of all jobs");
  } else {
    m_logger->info("Requested status of Job ID: {}\n", job_id);
  }
  string header_string;
  for (const auto& h : headers) {
    header_string += h.first +" -- " + h.second +"\n";
  }
  m_logger->trace("Recieved headers:\n{}", header_string);
  SessionResponse response;
  try {
    const auto status{ServerHandler::cget().get_job_status(job_id)};
    response.return_code = restbed::OK;
    response.body = status;
  } catch (const exception& e) {
    response.return_code = restbed::BAD_REQUEST;
    response.body = "Invalid job id";
  }
  response.headers = {{"Content-Length", to_string(response.body.length())},
                      {"Connection", "Close"}};
  session->close(response.return_code, response.body, response.headers);
}
}  // namespace sel
