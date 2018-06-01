/**
\file    jsonmethodhandler.cpp
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
\brief Parses and validates requests and generates JSON object from body
*/

#include "jsonmethodhandler.h"
#include "restbed"
#include "fmt/format.h"
#include <memory>
#include <string>

//void sel::JsonMethodHandler::handle_continue(

    //std::shared_ptr<restbed::Session> session) const {
  //auto request{session->get_request()};
  //auto headers{request->get_headers()};
  //std::string remote_id{request->get_path_parameter("remote_id", "")};
  //size_t content_length = request->get_header("Content-Length", 0);
  //fmt::print("Remote ID: {}", remote_id);
  //fmt::print("Continue Recieved headers:\n");
  //for (const auto& h : headers) {
    //fmt::print("{} -- {}\n", h.first, h.second);
  //}
  //if (content_length) {
    //session->fetch(
        //content_length,
        //[&](const std::shared_ptr<restbed::Session> session[[maybe_unused]],
            //const restbed::Bytes& body) {
          //std::string bodystring = std::string(body.begin(), body.end());
          //nlohmann::json data = nlohmann::json::parse(bodystring);
          //use_data(session, data, remote_id);
        //});
  //} else {
    //session->close(restbed::LENGTH_REQUIRED);
  //}
//}

void sel::JsonMethodHandler::handle_method(
    std::shared_ptr<restbed::Session> session) const {
  auto request{session->get_request()};
  auto headers{request->get_headers()};
  sel::RemoteId remote_id{request->get_path_parameter("remote_id", "")};
  size_t content_length = request->get_header("Content-Length", 0);
  fmt::print("Remote ID: {}\n", remote_id);
  fmt::print("Recieved headers:\n");
  for (const auto& h : headers) {
    fmt::print("{} -- {}\n", h.first, h.second);
  }
  //if (request->get_header("Expect", restbed::String::lowercase) ==
      //"100-continue") {
    //fmt::print("Continuing\n");
    //auto fun = std::bind(&sel::JsonMethodHandler::handle_continue, this,
                         //std::placeholders::_1);
    //session->yield(restbed::CONTINUE, restbed::Bytes(), fun);
  //}
  if (content_length) {
    session->fetch(
        content_length,
        [=](const std::shared_ptr<restbed::Session> session[[maybe_unused]],
            const restbed::Bytes& body) {
          std::string bodystring = std::string(body.begin(), body.end());
          nlohmann::json data = nlohmann::json::parse(bodystring);
          use_data(session, data, remote_id);
        });
  } else {
    session->close(restbed::LENGTH_REQUIRED);
  }
}

void sel::JsonMethodHandler::use_data(
    const std::shared_ptr<restbed::Session>& session,
    const nlohmann::json& bodydata, const sel::RemoteId& remote_id) const {
#ifdef DEBUG
  fmt::print("Data recieved:\n{}\n", bodydata.dump(4));
#endif
  auto validation = m_validator->validate_json(bodydata);
  SessionResponse response;
  if (validation.first) {
    if (m_valid_callback) {
      response = m_valid_callback(bodydata,remote_id, m_connection_handler);
    } else {
      throw std::runtime_error("Invalid valid_callback!");
    }
  } else {
    if (m_invalid_callback) {
      response = m_invalid_callback(validation.second);
    } else {
      throw std::runtime_error("Invalid invalid_callback");
    }
  }
  session->close(response.return_code,response.body ,response.headers);
}
