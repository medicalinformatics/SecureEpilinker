/**
\file    restresponses.hpp
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
\brief Defines standard REST responses to send out
*/

#include "resttypes.h"
#include "corvusoft/restbed/status_code.hpp"

namespace sel{
  namespace responses{
  SessionResponse server_initialized(Port port){
    return{restbed::OK, "Connection Initialized", {{"Content-Length", "22"},{"Connection", "Close"}, {"SEL-Port", std::to_string(port)}}};
  }
  SessionResponse status_error(int status, std::string msg) {
  return {status, msg, {{"Content-Length", std::to_string(msg.length())},
                 {"Connection", "Close"}}};
  }
  SessionResponse not_initialized{restbed::UNAUTHORIZED, "No connection initialized", {{"Content-Length", "25"}, {"Connection", "Close"}}}; 

  } // namespace responses

} // namespace sel
