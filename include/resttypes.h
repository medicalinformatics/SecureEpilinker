/**
\file    resttypes.h
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

#ifndef SEL_RESTTYPES_H
#define SEL_RESTTYPES_H
#pragma once

#include <map>
#include <memory>
#include <string>

namespace sel{

// Forward Declarations
class AuthenticationConfig;

using JobId = std::string;
using RemoteId = std::string;
using ClientId = std::string;
using ToDate = size_t;

enum class AlgorithmType { EPILINK };
enum class AuthenticationType { NONE, API_KEY };
enum class JobStatus { QUEUED, RUNNING, HOLD, FAULT, DONE };

AlgorithmType str_to_atype(const std::string& str);
AuthenticationType str_to_authtype(const std::string& str);
std::string js_enum_to_string(JobStatus);

struct ConnectionConfig {
  std::string url;
  std::unique_ptr<AuthenticationConfig> authentication;
};

struct CallbackConfig {
  std::string url;
  std::string token;
};

struct AlgorithmConfig {
  AlgorithmType type;
  unsigned bloom_length;
  double threshold_match;
  double threshold_non_match;
};

struct SessionResponse {
  int return_code;
  std::string body;
  std::multimap<std::string, std::string> headers;
};
} // namespace sel
#endif /* end of include guard: SEL_RESTTYPES_HPP */
