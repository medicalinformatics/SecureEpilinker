/**
\file    jsonutils.cpp
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
  \brief utility functions regarding json data
*/

#include "jsonutils.h"
#include "logger.h"
#include "util.h"
#include "base64.h"
#include <fstream>

using namespace std;

namespace sel {

Bitmask check_size_and_get_as_bitmask(
    const void* source, const size_t source_bytes, const size_t bytes_to_copy) {
  if (bytes_to_copy > source_bytes) {
    throw runtime_error("Field bitlength larger than field type, "
        "cannot copy that many bytes!");
  }
  Bitmask ret(bytes_to_copy);
  ::memcpy(ret.data(), source, bytes_to_copy);

  return ret;
}

void check_bitsize_and_clear_extra_bits(std::vector<uint8_t>& bitmask,
    const size_t size) {
  if (!(bitbytes(size) == bitmask.size()))
    throw new runtime_error("Bitmask size mismatch in check_bitsize_and_clear_extra_bits()");

  const unsigned char extrabits = size % 8u;
  auto& rear = bitmask.back();
  if (extrabits && (rear >> extrabits)) {  // Extra bits set outside
    rear &= (1u << extrabits) - 1u;
    get_default_logger()->warn(
        "Bits set after bitmask's size, setting to zero.\n");
  }
}

FieldEntry parse_json_field(const ML_Field& field,
                                            const nlohmann::json& json) {
  auto logger{get_default_logger()};
  if (!(json.is_null())) {
    size_t field_bytes = bitbytes(field.bitsize);
    switch (field.type) {
      case FieldType::INTEGER: {
        const auto content = json.get<int>();
        return check_size_and_get_as_bitmask(&content, sizeof(int), field_bytes);
      }
      case FieldType::NUMBER: {
        const auto content = json.get<double>();
        return check_size_and_get_as_bitmask(&content, sizeof(double), field_bytes);
      }
      case FieldType::STRING: {
        const auto content = json.get<string>();
        if (trim_copy(content).empty()) {
          return nullopt;
        } else {
          return check_size_and_get_as_bitmask(content.c_str(), content.size(),
              field_bytes);
        }
      }
      case FieldType::BITMASK: {
        const auto bloom_base64 = json.get<string>();
        if (!trim_copy(bloom_base64).empty()) {
          auto bloom = base64_decode(bloom_base64, field.bitsize);
          check_bitsize_and_clear_extra_bits(bloom, field.bitsize);
          return bloom;
        } else {
          return nullopt;
        }
      }
      default: {
        // silence compiler, never reached
        return nullopt;
      }
    }
  } else {  // field has type null
    return nullopt;
  }
}

map<FieldName, FieldEntry> parse_json_fields(
    const map<FieldName, ML_Field>& fields, const nlohmann::json& json) {
  map<FieldName, FieldEntry> result;
  for (auto f = json.cbegin(); f != json.cend(); ++f) {
    auto entry = parse_json_field(fields.at(f.key()), *f);
    result.emplace(f.key(), move(entry));
  }
  return result;
}

map<FieldName, vector<FieldEntry>> parse_json_fields_array(
    const map<FieldName, ML_Field>& fields, const nlohmann::json& json) {
  map<FieldName, VFieldEntry> records;
  for (const auto& rec : json) {
    if (!rec.count("fields")) {
      throw runtime_error("Invalid JSON Data: missing 'fields' in records array");
    }

    auto data_fields = parse_json_fields(fields, rec.at("fields"));
    for (auto& fields : data_fields){
      records[fields.first].emplace_back(move(fields.second));
    }
  }

  return records;
}

vector<string> parse_json_id_array(const nlohmann::json& json) {
  vector<string> ids;
  for (const auto& rec : json) {
    if (!rec.count("id")) {
      throw runtime_error("Invalid JSON Data: missing 'id' in records array");
    }

    ids.emplace_back(rec.at("id").get<string>());
  }

  return ids;
}

map<FieldName, ML_Field> parse_json_fields_config(nlohmann::json fields_json) {
  map<FieldName, ML_Field> fields_config;
  for (const auto& f : fields_json) {
    const string field_name = f.at("name").get<string>();
    ML_Field tempfield(
        field_name, f.at("frequency").get<double>(),
        f.at("errorRate").get<double>(), f.at("comparator").get<string>(),
        f.at("fieldType").get<string>(), f.at("bitlength").get<size_t>());
    fields_config[field_name] = move(tempfield);
  }

  return fields_config;
}

vector<IndexSet> parse_json_exchange_groups(nlohmann::json xgroups_json) {
  vector<IndexSet> xgroups;
  xgroups.reserve(xgroups_json.size());
  for (const auto& xgroup_json : xgroups_json) {
    IndexSet xgroup;
    for (auto& f : xgroup_json) {
      xgroup.emplace(f.get<string>());
    }
    xgroups.emplace_back(xgroup);
  }

  return xgroups;
}

EpilinkConfig parse_json_epilink_config(nlohmann::json config_json) {
    const auto fields = parse_json_fields_config(config_json.at("fields"));
    const auto xgroups = parse_json_exchange_groups(config_json.at("exchangeGroups"));
    const double threshold = config_json.at("threshold_match").get<double>();
    const double tthreshold = config_json.at("threshold_non_match").get<double>();
    return {fields, xgroups, threshold, tthreshold};
}

nlohmann::json read_json_from_disk(
    const filesystem::path& json_path) {
  auto logger{get_default_logger()};
  nlohmann::json content;
  if (filesystem::exists(json_path)) {
    ifstream in(json_path);
    try {
      in >> content;
    } catch (const std::ios_base::failure& e) {
      logger->error("Reading of file {} failed: {}", json_path.string(),
                    e.what());
      exit(1);
    }
    return content;
  } else {
    throw runtime_error(json_path.string() + " does not exist!");
    return content;  // To silence warning, will never be executed
  }
}

} /* END namespace sel */
