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
#include <cassert>
#include <stdexcept>
#include <string>

using namespace std;
namespace sel {
FieldType str_to_ftype(const string& str) {
  if (str == "bitmask")
    return FieldType::BITMASK;
  else if (str == "number")
    return FieldType::NUMBER;
  else if (str == "string")
    return FieldType::STRING;
  else if (str == "integer")
    return FieldType::INTEGER;
  assert(!"This should never be reached!");
  throw runtime_error("Invalid Field Type");
}

FieldComparator str_to_fcomp(const string& str) {
  if (str == "nGram")
    return FieldComparator::NGRAM;
  else if (str == "binary")
    return FieldComparator::BINARY;
  assert(!"This should never be reached!");
  throw runtime_error("Invalid Comparator Type");
}
}  // namespace sel
