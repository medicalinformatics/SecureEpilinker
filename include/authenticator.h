/**
\file    authenticator.h
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
\brief holds authentication info and implements signing and verifying of transactions
*/

#ifndef SEL_AUTHENTICATOR_H
#define SEL_AUTHENTICATOR_H
#pragma once

#include <string>
#include <memory>
#include "resttypes.h"


namespace sel{
class Authenticator {
  public:
    explicit Authenticator(AuthenticationConfig);
    explicit Authenticator(AuthenticationConfig&&);
    Authenticator() = default;
    Authenticator(const Authenticator&) = default;
    Authenticator(Authenticator&&) = default;
    Authenticator& operator=(Authenticator&&) = default;

    void set_auth_info(std::unique_ptr<AuthenticationConfig>);

    std::string sign_transaction(const std::string&) const;
    bool verify_transaction(const std::string&) const;

    AuthenticationType get_auth_type() const;
    std::string print_auth_type() const;
    SessionResponse check_authentication_header(const std::multimap<std::string, std::string>&) const;
    SessionResponse check_authentication(const std::string&) const;
  private:
    std::unique_ptr<AuthenticationConfig> m_auth_config;
};

} // namespace sel
#endif /* end of include guard: SEL_AUTHENTICATOR_H */
