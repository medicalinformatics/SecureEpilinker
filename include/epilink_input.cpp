/**
 \file    epilink_input.cpp
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
 \brief Encapsulation of EpiLink Algorithm Inputs
*/

#include <memory>
#include <algorithm>
#include <fmt/format.h>
#include <iostream>
#include "epilink_input.h"
#include "util.h"
#include "math.h"
#include "logger.h"

using namespace std;
using fmt::print;

namespace sel {

EpilinkConfig::EpilinkConfig(
      std::map<FieldName, ML_Field> fields_,
      std::vector<IndexSet> exchange_groups_,
      double threshold, double tthreshold) :
  fields {fields_},
  exchange_groups {exchange_groups_},
  threshold {threshold},
  tthreshold {tthreshold},
  nfields {fields.size()},
  max_weight{sel::max_element(fields, [](auto f){return f.second.weight;})}
{
  auto logger = get_default_logger();
  logger->trace("Constructing {}", *this);

  // Sanity checks of exchange groups
  IndexSet xgunion;
  for (const auto& group : exchange_groups) {
    const auto& f0 = fields.at(*group.cbegin());
    for (const auto& fname : group) {
      // Check that field is configured
      if (!fields.count(fname)) throw invalid_argument(fmt::format(
          "Exchange groups contains non-existing field '{}'!", fname));


      // Check if exchange groups are disjoint
      auto [_, is_new] = xgunion.insert(fname);
      if (!is_new) throw invalid_argument(fmt::format(
          "Exchange groups must be distinct! Field {} specified multiple times.", fname));

      const auto& f = fields.at(fname);

      // Check same comparators
      if (f.comparator != f0.comparator) {
        throw invalid_argument{fmt::format(
            "Cannot compare field '{}' of type {} with field '{}' of type {}",
            f.name, f.comparator, f0.name, f0.comparator)};
      }

      // Check same bitsize
      if (f.bitsize != f0.bitsize) {
        throw invalid_argument{fmt::format(
            "Cannot compare field '{}' of bitsize {} with field '{}' of bitsize {}",
            f.name, f.bitsize, f0.name, f0.bitsize)};
      }
    }
  }

  for (const auto& f : fields) {
    auto& field = f.second;
    if (field.type == FieldType::STRING && field.bitsize%8) {
      logger->warn("String field '{}' has bitsize not divisible by 8.");
    }
  }
}

void EpilinkServerInput::check_sizes() {
  for (const auto& row : database) {
    check_vector_size(row.second, nvals, "database field "s + row.first);
  }
}

EpilinkServerInput::EpilinkServerInput(
    const std::map<FieldName, VFieldEntry>& database_) :
  database(database_),
  nvals {database.cbegin()->second.size()}
{ check_sizes(); }

EpilinkServerInput::EpilinkServerInput(
    std::map<FieldName, VFieldEntry>&& database_) :
  database{move(database_)},
  nvals {database.cbegin()->second.size()}
{ check_sizes(); }


} // namespace sel

std::ostream& operator<<(std::ostream& os,
    const sel::FieldEntry& val) {
  if (val) {
    for (const auto& b : *val) os << b;
  } else {
    os << "<empty>";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os,
    const std::pair<const sel::FieldName, sel::FieldEntry>& f) {
  return os << f.first << ": " << f.second;
}

std::ostream& operator<<(std::ostream& os, const sel::EpilinkClientInput& in) {
  os << "----- Client Input -----\n";
  for (const auto& f : in.record) {
    os << f << '\n';
  }
  return os << "Number of database records: " << in.nvals;
}

std::ostream& operator<<(std::ostream& os, const sel::EpilinkServerInput& in) {
  os << "----- Server Input -----\n";
  for (const auto& fs : in.database) {
    for (size_t i = 0; i != fs.second.size(); ++i) {
      os << fs.first << '[' << i << "]: " << fs.second[i] << '\n';
    }
  }
  return os << "Number of database records: " << in.nvals;
}
