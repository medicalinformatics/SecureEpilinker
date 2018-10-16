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
using fmt::format;

namespace sel {

EpilinkConfig::EpilinkConfig(
      std::map<FieldName, FieldSpec> fields_,
      std::vector<IndexSet> exchange_groups_,
      double threshold, double tthreshold) :
  fields {fields_},
  exchange_groups {exchange_groups_},
  threshold {threshold},
  tthreshold {tthreshold},
  nfields {fields.size()},
  max_weight{sel::max_element(fields, [](auto f){return f.second.weight;})}
{
  auto logger = get_logger();
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

EpilinkClientInput::EpilinkClientInput(unique_ptr<Records>&& records_, size_t database_size_) :
  records{move(records_)},
  database_size {database_size_},
  num_records {records->size()}
{ check_keys(); }

EpilinkClientInput::EpilinkClientInput(const Record& record, size_t database_size_) :
  records{new Records{record}},
  database_size {database_size_},
  num_records {1}
{ check_keys(); }

void EpilinkClientInput::check_keys() {
  const auto first_keys = map_keys(*records->cbegin());
  for (const auto& record : *records) {
    const auto keys = map_keys(record);
    if (!(keys == first_keys)) {
      throw invalid_argument(format("EpilinkClientInput: Keys of input maps "
            "are inconsistent. Found {} and {}.", first_keys, keys));
    }
  }
}

void EpilinkServerInput::check_sizes() {
  for (const auto& row : *database) {
    check_vector_size(row.second, database_size, "database field "s + row.first);
  }
}

EpilinkServerInput::EpilinkServerInput(shared_ptr<VRecord> database_, size_t num_records_) :
  database(move(database_)),
  database_size {database->cbegin()->second.size()},
  num_records {num_records_}
{ check_sizes(); }

EpilinkServerInput::EpilinkServerInput(const VRecord& database_, size_t num_records_) :
  database(make_shared<VRecord>(database_)),
  database_size {database->cbegin()->second.size()},
  num_records {num_records_}
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
  const auto& records = *(in.records);
  for (size_t i = 0; i != records.size(); ++i) {
    os << '[' << i << "] " << records[i];
  }
  os << "Number of records to link: " << in.num_records << '\n';
  return os << "Number of database records: " << in.database_size;
}

std::ostream& operator<<(std::ostream& os, const sel::EpilinkServerInput& in) {
  os << "----- Server Input -----\n";
  for (const auto& fs : *(in.database)) {
    for (size_t i = 0; i != fs.second.size(); ++i) {
      os << fs.first << '[' << i << "]: " << fs.second[i] << '\n';
    }
  }
  os << "Number of records to link: " << in.num_records << '\n';
  return os << "Number of database records: " << in.database_size;
}
