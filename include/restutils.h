/**
\file    restutils.h
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
  \brief utility functions regarding the secure epilinker rest interface
*/

#ifndef SEL_RESTUTILS_H
#define SEL_RESTUTILS_H

#include "localconfiguration.h"
#include "remoteconfiguration.h"
#include <memory>


namespace sel{
std::string assemble_remote_url(const std::shared_ptr<const RemoteConfiguration>&);
std::string assemble_remote_url(RemoteConfiguration const * );
SessionResponse perform_post_request(std::string, std::string, std::list<std::string>, bool);
SessionResponse perform_get_request(std::string, std::list<std::string>, bool);
SessionResponse send_result_to_linkageservice(const SecureEpilinker::Result&,const std::string&,const std::shared_ptr<const LocalConfiguration>&,const std::shared_ptr<const RemoteConfiguration>&);
std::vector<std::string> get_headers(std::istream& is,const std::string& header);
std::vector<std::string> get_headers(const std::string&,const std::string& header);
} // namespace sel
#endif /* end of include guard: SEL_RESTUTILS_H */
