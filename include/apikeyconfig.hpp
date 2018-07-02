/**
\file    apikeyconfig.hpp
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
\brief From AuthenticationConfig derived class to store nessecary nformation
    for shared API key authentication
*/

#ifndef SEL_APIKEYCONFIG_H
#define SEL_APIKEYCONFIG_H
#pragma once

#include "authenticationconfig.hpp"
#include "resttypes.h"


namespace sel {

class APIKeyConfig : public AuthenticationConfig {
  /**
   * From Authentication_Config derived class to store all nessecary information
   * for a shared API key authentication (i.e. the key)
   */
 public:
  explicit APIKeyConfig(AuthenticationType type)
      : AuthenticationConfig(type) {}
  explicit APIKeyConfig(AuthenticationType type, std::string key)
      : AuthenticationConfig(type), m_shared_key(std::move(key)) {}

  const std::string get_key() const { return m_shared_key; }

 private:
  std::string m_shared_key;
};

}  // namespace sel

#endif  // SEL_APIKEYCONFIG_H
