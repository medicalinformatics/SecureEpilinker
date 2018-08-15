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
  \brief utility functions regarding the secure epilinker rest inteerface
*/

#ifndef SEL_RESTUTILS_H
#define SEL_RESTUTILS_H

#include "seltypes.h"
#include "epilink_input.h"
#include <tuple>
#include "nlohmann/json.hpp"
#include "localconfiguration.h"
#include <experimental/filesystem>
#include <memory>



namespace sel{
std::pair<FieldName, FieldEntry> parse_json_field(const ML_Field&, const nlohmann::json&);
std::map<FieldName,FieldEntry> parse_json_fields(std::shared_ptr<const LocalConfiguration>, const nlohmann::json&);
nlohmann::json read_json_from_disk(const std::experimental::filesystem::path&);
std::vector<std::string> get_headers(std::istream& is,const std::string& header);
std::vector<std::string> get_headers(const std::string&,const std::string& header);
} // namespace sel
#endif /* end of include guard: SEL_RESTUTILS_H */
