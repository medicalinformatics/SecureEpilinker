/**
\file    authenticator.cpp
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
\brief holds authentication info and implements signing and verifying of
transactions
*/

#include "authenticator.h"
#include <exception>
#include <memory>
#include <string>
#include "apikeyconfig.hpp"
#include "authenticationconfig.hpp"
#include "logger.h"
#include "restresponses.hpp"
#include "resttypes.h"
#include "util.h"

using namespace std;
namespace sel {

Authenticator::Authenticator(AuthenticationConfig auth_config)
    : m_auth_config{std::make_unique<AuthenticationConfig>(move(auth_config))} {
}
Authenticator::Authenticator(AuthenticationConfig&& auth_config)
    : m_auth_config{std::make_unique<AuthenticationConfig>(move(auth_config))} {
}

Authenticator::Authenticator(unique_ptr<AuthenticationConfig> auth_config) :
      m_auth_config{move(auth_config)} {}

void Authenticator::set_auth_info(unique_ptr<AuthenticationConfig> auth_info) {
  if (!m_auth_config) {
    m_auth_config = move(auth_info);
  } else {
    throw logic_error(
        "You can not modify the authentication config after initialization");
  }
}

AuthenticationType Authenticator::get_auth_type() const {
  return m_auth_config->get_type();
}

string Authenticator::print_auth_type() const {
  return m_auth_config->print_type();
}

bool Authenticator::verify_transaction(const string& signature) const {
  switch (m_auth_config->get_type()) {
    case AuthenticationType::API_KEY: {
      auto conf{dynamic_cast<APIKeyConfig*>(m_auth_config.get())};
      return signature == conf->get_key();
    }
    case AuthenticationType::NONE: {
      return true;
    }
  }
}

SessionResponse Authenticator::check_authentication_header(
    const std::multimap<std::string, std::string>& header) const {
  auto logger = get_logger();
  try {
    string auth_info;
    if (auto it = header.find("Authorization"); it != header.end()) {
      auth_info = it->second;
      return check_authentication(auth_info);
    } else {  // No auth header
      auto type{print_auth_type()};
      logger->warn("Unauthorized request, no headerj");
      return responses::unauthorized(type);
    }
  } catch (const exception& e) {
    logger->error("Error authenticating request");
    return responses::status_error(500, e.what());
  }
}

SessionResponse Authenticator::check_authentication(const string& auth_info) const {
  auto logger = get_logger();
  try {
    auto expected_type{print_auth_type()};
    string given_type(auth_info.begin(),
                      next(auth_info.begin(), expected_type.length()));
    logger->debug("Expected Auth Type: \"{}\", Given: \"{}\"", expected_type,
                  given_type);
    if (given_type == expected_type) {
      // auth_info is of form apiKey apiKey="<APIKey>"
      auto values{split(auth_info, '"')};
      // values are of form ["apiKey apiKey=","<APIKey>",""]
      if (!verify_transaction(values.at(1))) {  // Invalid Key
        logger->warn("Forbidden request");
        return responses::status_error(403, "Not authorized");
      }
    } else {  // Invalid Auth type
      logger->warn("Unauthorized request");
      return responses::unauthorized(expected_type);
    }
  } catch (const exception& e) {
    logger->error("Error authenticating request");
    return responses::status_error(500, e.what());
  }
  return {200, "", {{}}};
}

string Authenticator::sign_transaction(const string& msg) const{
  switch (m_auth_config->get_type()) {
    case AuthenticationType::API_KEY: {
      auto conf{dynamic_cast<APIKeyConfig*>(m_auth_config.get())};
      return "apiKey apiKey=\""s+conf->get_key()+"\"";
    }
    case AuthenticationType::NONE: {
      return "";
    }
  }
}
}  // namespace sel
