/**
\file    authenticationconfig.hpp
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
\brief Baseclass and factory for authentication information objects. Holds the
    authentication type.
*/

#ifndef SEL_AUTHENTICATIONCONFIG_HPP
#define SEL_AUTHENTICATIONCONFIG_HPP
#pragma once

#include "resttypes.h"
#include <memory>
#include <string>


namespace sel {


class AuthenticationConfig {
  /**
   * Base class for specific authentication configuration
   */

 public:
  AuthenticationConfig() = default;
  AuthenticationConfig(const AuthenticationConfig&) = default;
  explicit AuthenticationConfig(AuthenticationType type) : m_type(type) {}
  virtual ~AuthenticationConfig() = default;
  AuthenticationType get_type() const  { return m_type; }
  std::string print_type() const {
    switch(m_type){
      case AuthenticationType::NONE: return "none"; break;
      case AuthenticationType::API_KEY: return "apiKey"; break;
    }
    return "Unknown Type! That's an error!";
  }

  template <typename T, typename... Args>
  static std::unique_ptr<AuthenticationConfig> create_authentication(Args... arguments){
    return std::unique_ptr<AuthenticationConfig>(new T(arguments ...));
  }

 private:
  AuthenticationType m_type{AuthenticationType::NONE};
};

}  // namespace sel
#endif // SEL_AUTHENTICATIONCONFIG_HPP
