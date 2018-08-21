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
    // FIXME(TK) I do s.th. *very* unsafe and use bitlength user input directly
    // for memcpy. DO SOME SANITY CHECKS OR THIS SOFTWARE WILL BREAK AND ALLOW
    // ARBITRARY REMOTE CODE EXECUTION!
pair<FieldName, FieldEntry> parse_json_field(const ML_Field& field,
                                            const nlohmann::json& json) {
  auto logger{get_default_logger()};
  if (!(json.is_null())) {
    switch (field.type) {
      case FieldType::INTEGER: {
        const auto content{json.get<int>()};
        Bitmask temp(bitbytes(field.bitsize));
        ::memcpy(temp.data(), &content, bitbytes(field.bitsize));
        return make_pair(field.name, move(temp));
      }
      case FieldType::NUMBER: {
        const auto content{json.get<double>()};
        Bitmask temp(bitbytes(field.bitsize));
        ::memcpy(temp.data(), &content, bitbytes(field.bitsize));
        return make_pair(field.name, move(temp));
      }
      case FieldType::STRING: {
        const auto content{json.get<string>()};
        if (trim_copy(content).empty()) {
          return make_pair(field.name, nullopt);
        } else {
          const auto temp_char_array{content.c_str()};
          Bitmask temp(bitbytes(field.bitsize));
          ::memcpy(temp.data(), temp_char_array, bitbytes(field.bitsize));
          return make_pair(field.name, move(temp));
        }
      }
      case FieldType::BITMASK: {
        auto temp = json.get<string>();
        if (!trim_copy(temp).empty()) {
          auto bloom = base64_decode(temp, field.bitsize);
          if (!check_bloom_length(bloom, field.bitsize)) {
            logger->warn(
                "Bits set after bloomfilter length. There might be a "
                "problem. Setting to zero.\n");
          }
          return make_pair(field.name, move(bloom));
        } else {
          return make_pair(field.name, nullopt);
        }
      }
      default: {
        // silence compiler, never reached
        return make_pair("", nullopt);
      }
    }
  } else {  // field has type null
    return make_pair(field.name, nullopt);
  }
}

map<FieldName, FieldEntry> parse_json_fields(
    const map<FieldName, ML_Field>& fields, const nlohmann::json& json) {
  map<FieldName, FieldEntry> result;
  for (auto f = json.at("fields").begin(); f != json.at("fields").end(); ++f) {
    result[f.key()] = move(parse_json_field(fields.at(f.key()), *f).second);
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

    auto data_fields = parse_json_fields(fields ,rec);
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
