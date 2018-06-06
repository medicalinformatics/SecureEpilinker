/**
\file    seltypes.cpp
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
\brief Project specific types and enum convenience functions
*/

#include "seltypes.h"
#include <string>
#include <stdexcept>
#include <cassert>

using sel::FieldType;
using sel::FieldComparator;
using sel::AlgorithmType;
using sel::AuthenticationType;
using sel::JobStatus;

FieldType sel::str_to_ftype(const std::string& str) {
    if(str =="bitmask") return FieldType::BITMASK;
    else if(str =="number") return FieldType::NUMBER;
    else if(str =="string") return FieldType::STRING;
    else if(str =="integer") return FieldType::INTEGER;
    assert(!"This should never be reached!");
    throw std::runtime_error("Invalid Field Type");
}

FieldComparator sel::str_to_fcomp(const std::string& str) {
    if(str =="nGram") return FieldComparator::NGRAM;
    else if(str =="binary") return FieldComparator::BINARY;
    assert(!"This should never be reached!");
    throw std::runtime_error("Invalid Comparator Type");
}
AlgorithmType sel::str_to_atype(const std::string& str) {
    if(str =="epilink") return AlgorithmType::EPILINK;
    assert(!"This should never be reached!");
    throw std::runtime_error("Invalid Algorithm Type");
}
AuthenticationType sel::str_to_authtype(const std::string& str) {
    if(str =="apikey") return AuthenticationType::API_KEY;
    else if(str =="none") return AuthenticationType::NONE;
    assert(!"This should never be reached!");
    throw std::runtime_error("Invalid Authentication Type");
}

std::string sel::js_enum_to_string(JobStatus status){
  switch (status){
    case JobStatus::RUNNING: {return "Running"; break;}
    case JobStatus::QUEUED:  {return "Queued"; break;}
    case JobStatus::DONE:    {return "Done"; break;}
    case JobStatus::HOLD:    {return "Hold"; break;}
    case JobStatus::FAULT:   {return "Fault"; break;}
    default:                 {throw std::runtime_error("Invalid Status"); 
                              return "Error!";}
  }
}
