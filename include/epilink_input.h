/**
 \file    epilink_input.h
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
 \brief Encapsulation of cleartext EpiLink algorithm inputs
*/

#ifndef SEL_EPILINKINPUT_H
#define SEL_EPILINKINPUT_H
#pragma once

#include "seltypes.h"
#include "fmt/ostream.h"
#include "util.h"
#include <vector>
#include <map>
#include <cstdint>
#include <optional>

namespace sel {

// dice/bitmask field types of which hamming weights are computed
using BitmaskUnit = uint8_t;
using Bitmask = std::vector<BitmaskUnit>;
using VBitmask = std::vector<Bitmask>;
// How we save all input data
using FieldEntry = std::optional<Bitmask>;
using VFieldEntry = std::vector<FieldEntry>;
using Record = std::map<FieldName, FieldEntry>;
using VRecord = std::map<FieldName, VFieldEntry>;
using Records = std::vector<Record>;

struct EpilinkConfig {
  // field descriptions
  std::map<FieldName, FieldSpec> fields;

  // exchange groups by index
  std::vector<IndexSet> exchange_groups;

  // thresholds
  double threshold; // threshold for definitive match
  double tthreshold; // threshold for tentative match

  // pre-calculated fields
  size_t nfields; // total number of field
  Weight max_weight; // maximum weight for rescaling of weights

  EpilinkConfig(
      std::map<FieldName, FieldSpec> fields,
      std::vector<IndexSet> exchange_groups,
      double threshold, double tthreshold
  );
  EpilinkConfig() = default;
  EpilinkConfig(const EpilinkConfig&) = default;
  EpilinkConfig(EpilinkConfig&&) = default;
  EpilinkConfig& operator=(const EpilinkConfig&) = default;
  EpilinkConfig& operator=(EpilinkConfig&&) = default;
  ~EpilinkConfig() = default;
};

struct EpilinkClientInput {
  // Outer vector by fields, inner by records!
  // nfields map of vec input records to link
  std::unique_ptr<Records> records;

  // need to know database size of remote server when building circuit
  size_t database_size;
  size_t num_records; // calculated

  EpilinkClientInput(std::unique_ptr<Records>&& records, size_t database_size);
  EpilinkClientInput(const Record& record, size_t database_size);
  EpilinkClientInput(EpilinkClientInput&&) = default;
  EpilinkClientInput& operator=(EpilinkClientInput&&) = default;
  ~EpilinkClientInput() = default;
private:
  void check_keys(); // called by public constructors to check that keys match
};

struct EpilinkServerInput {
  // Outer vector by fields, inner by records!
  // Need to model like this for ABY SIMD layout
  std::shared_ptr<VRecord> database;

  size_t database_size; // calculated
  // need to know number of remote client records when building circuit
  size_t num_records;

  EpilinkServerInput(std::shared_ptr<VRecord> database, size_t num_records);
  EpilinkServerInput(const VRecord& database, size_t num_records);
  EpilinkServerInput(const EpilinkServerInput&) = default;
  EpilinkServerInput(EpilinkServerInput&&) = default;
  EpilinkServerInput& operator=(const EpilinkServerInput&) = default;
  EpilinkServerInput& operator=(EpilinkServerInput&&) = default;
  ~EpilinkServerInput() = default;
private:
  void check_sizes(); // called by public constructors to check sizes
};

} // namespace sel

std::ostream& operator<<(std::ostream& os,
    const std::pair<const sel::FieldName, sel::FieldEntry>& f);
std::ostream& operator<<(std::ostream& os, const sel::FieldEntry& val);
std::ostream& operator<<(std::ostream& os, const sel::EpilinkClientInput& in);
std::ostream& operator<<(std::ostream& os, const sel::EpilinkServerInput& in);

// Custom fmt formatters for our types
namespace fmt {

template <>
struct formatter<sel::EpilinkConfig> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const sel::EpilinkConfig& conf, FormatContext &ctx) {
    const auto field_names = map_keys(conf.fields);
    return format_to(ctx.begin(),
        "EpilinkConfig{{thresholds={};{}, nfields={}, fields={}}}",
        conf.threshold, conf.tthreshold, conf.nfields, field_names
    );
  }
};

} // namespace fmt

#endif /* end of include guard: SEL_EPILINKINPUT_H */
