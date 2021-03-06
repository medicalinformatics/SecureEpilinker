/**
\file    resttypes.cpp
\author  Tobias Kussel <kussel@cbs.tu-darmstadt.de>
\author  Sebastian Stammler <sebastian.stammler@cysec.de>
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
\brief REST interface specific types and enum convenience functions
*/

#include "resttypes.h"
#include <cassert>
#include <stdexcept>
#include <string>

#include <future>
#include <sstream>
#include <curlpp/Easy.hpp>
#include <curlpp/Infos.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>
using std::string;
using std::runtime_error;

namespace sel {

AlgorithmType str_to_atype(const string& str) {
  if (str == "epilink")
    return AlgorithmType::EPILINK;
  assert(!"This should never be reached!");
  throw runtime_error("Invalid Algorithm Type");
}

AuthenticationType str_to_authtype(const string& str) {
  if (str == "apiKey")
    return AuthenticationType::API_KEY;
  else if (str == "none")
    return AuthenticationType::NONE;
  assert(!"This should never be reached!");
  throw runtime_error("Invalid Authentication Type");
}

string js_enum_to_string(JobStatus status) {
  switch (status) {
    case JobStatus::RUNNING: {
      return "Running";
    }
    case JobStatus::QUEUED: {
      return "Queued";
    }
    case JobStatus::DONE: {
      return "Done";
    }
    case JobStatus::HOLD: {
      return "Hold";
    }
    case JobStatus::FAULT: {
      return "Fault";
    }
    default: {
      throw runtime_error("Invalid Status");
      return "Error!";
    }
  }
}

} //namespace sel
