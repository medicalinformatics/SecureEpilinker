#include "cxxopts.hpp"
#include "fmt/format.h"
#include "abycore/aby/abyparty.h"

#include "../include/logger.h"
#include "../include/util.h"
#include "../include/jsonutils.h"
#include "../include/secure_epilinker.h"
#include "../include/clear_epilinker.h"
#include "random_input_generator.h"

#include <filesystem>

using namespace std;
using fmt::print;
using nlohmann::json;

namespace fs = std::filesystem;

namespace sel::test {

using Result = SecureEpilinker::Result;

constexpr auto BIN = FieldComparator::BINARY;
constexpr auto BM = FieldComparator::DICE;
constexpr double Threshold = 0.9;
constexpr double TThreshold = 0.7;

ML_Field f_int1 (
  //name, f, e, comparator, type, bitsize
  "int_1", 1.0,
  FieldComparator::BINARY, FieldType::INTEGER, 29
);

Bitmask data_int_1 = {0xde, 0xad, 0xbe, 0xef};

ML_Field f_int2 (
  //name, f, e, comparator, type, bitsize
  "int_2", 3.0,
  FieldComparator::BINARY, FieldType::INTEGER, 32
);

Bitmask data_int_2 = {0xde, 0xce, 0xa5, 0xed};

ML_Field f_bm1 (
  //name, f, e, comparator, type, bitsize
  "bm_1", 2.0,
  FieldComparator::DICE, FieldType::BITMASK, 8
);

ML_Field f_bm2 (
  //name, f, e, comparator, type, bitsize
  "bm_2", 4.0,
  FieldComparator::DICE, FieldType::BITMASK, 8
);

EpilinkConfig make_dkfz_cfg() {
  return {
    { // begin map<string, ML_Field>
      { "vorname",
        ML_Field("vorname", 0.000235, 0.01, "dice", "bitmask", 500) },
      { "nachname",
        ML_Field("nachname", 0.0000271, 0.008, "dice", "bitmask", 500) },
      { "geburtsname",
        ML_Field("geburtsname", 0.0000271, 0.008, "dice", "bitmask", 500) },
      { "geburtstag",
        ML_Field("geburtstag", 0.0333, 0.005, "binary", "integer", 5) },
      { "geburtsmonat",
        ML_Field("geburtsmonat", 0.0833, 0.002, "binary", "integer", 4) },
      { "geburtsjahr",
        ML_Field("geburtsjahr", 0.0286, 0.004, "binary", "integer", 11) },
      { "plz",
        ML_Field("plz", 0.01, 0.04, "binary", "string", 40) },
      { "ort",
        ML_Field("ort", 0.01, 0.04, "dice", "bitmask", 500) }
    }, // end map<string, ML_Field>
    { { "vorname", "nachname", "geburtsname" } }, // exchange groups
    Threshold, TThreshold
  };
}

// Whether to use run_as_{client/server}() or run_as_both()
bool run_both{false};
e_role role;

#ifdef DEBUG_SEL_CIRCUIT
Result run(SecureEpilinker& linker,
    const EpilinkClientInput& in_client, const EpilinkServerInput& in_server) {
  print("Calling run_as_{}()\n", run_both ? "both" : ((role==CLIENT) ? "client" : "server"));
  if (!run_both) {
    return (role==CLIENT) ?
      linker.run_as_client(in_client) : linker.run_as_server(in_server);
  } else {
    return linker.run_as_both(in_client, in_server);
  }
}
#endif

EpilinkInput input_simple(uint32_t nvals) {
  print("data_int_1: {}\n", data_int_1);

  // First test: only one bin field, single byte integer
  EpilinkConfig epi_cfg {
    { {"int_1", f_int1}, }, // fields
    {}, // exchange groups
    Threshold, TThreshold // size_bitmask, (tent.) thresholds
  };

  //Bitmask data_int_zero(data_int_1.size(), 0);

  EpilinkClientInput in_client {
    { {"int_1", data_int_1} }, // record
    nvals // nvals
  };

  EpilinkServerInput in_server {
    { {"int_1", vector<FieldEntry>(nvals, data_int_1)} } // records
  };

  return {epi_cfg, in_client, in_server};
}

EpilinkInput input_simple_bm(uint32_t nvals) {
  // Only one bm field, single byte integer
  EpilinkConfig epi_cfg {
    { {"bm_1", f_bm1}, }, // fields
    {}, // exchange groups
    Threshold, TThreshold // size_bitmask, (tent.) thresholds
  };

  //Bitmask data_int_zero(data_int_1.size(), 0);

  EpilinkClientInput in_client {
    { {"bm_1", Bitmask{0b01110111}} }, // record
    nvals // nvals
  };

  EpilinkServerInput in_server {
    { {"bm_1", vector<FieldEntry>(nvals, Bitmask{0b11101110})} } // records
  };

  return {epi_cfg, in_client, in_server};
}

EpilinkInput input_exchange_grp(uint32_t nvals) {
  // First test: only one bin field, single byte bitmask
  EpilinkConfig epi_cfg {
    {
      {"int_1", f_int1},
      {"int_2", f_int2},
      {"bm_1", f_bm1},
      {"bm_2", f_bm2},
    }, // fields
    {{"bm_1", "bm_2"}}, // exchange groups
    Threshold, TThreshold // size_bitmask, (tent.) thresholds
  };

  EpilinkClientInput in_client {
    {
      {"bm_1", Bitmask{0x33}},
      {"bm_2", Bitmask{0x43}},
      {"int_1", data_int_1},
      {"int_2", data_int_2}
    }, // record
    nvals // nvals
  };

  EpilinkServerInput in_server {
    {
      {"bm_1", vector<FieldEntry>(nvals, Bitmask{0x44})}, // 2-bit mismatch
      {"bm_2", vector<FieldEntry>(nvals, Bitmask{0x35})}, // 1-bit mismatch
      {"int_1", vector<FieldEntry>(nvals, data_int_1)},
      {"int_2", vector<FieldEntry>(nvals, data_int_2)}
    } // records
  };

  return {epi_cfg, in_client, in_server};
}

EpilinkInput input_empty(uint32_t nvals) {
  // First test: only one bin field, single byte bitmask
  EpilinkConfig epi_cfg {
    {
      {"bm_1", f_bm1},
      {"bm_2", f_bm2},
    }, // fields
    {}, // exchange groups
    Threshold, TThreshold // (tent.) thresholds
  };

  EpilinkClientInput in_client {
    {
      {"bm_1", nullopt},
      {"bm_2", Bitmask{0x44}},
    }, // record
    2 // nvals
  };

  EpilinkServerInput in_server {
    {
      {"bm_1", { nullopt, Bitmask{0x31} }}, // 1-bit mismatch for #1
      {"bm_2", { Bitmask{0x43}, Bitmask{0x44} }}, // 2-bit mismatch for #0
    } // records
  };

  return {epi_cfg, in_client, in_server};
}

EpilinkInput input_dkfz_random(size_t nvals) {
  RandomInputGenerator random_input(make_dkfz_cfg());
  random_input.set_client_empty_fields({"ort"});
  random_input.set_server_empty_field_probability(0);
  return random_input.generate(nvals);
}

EpilinkInput input_json(const fs::path& local_config_file_path,
    const fs::path& record_file_path,
    const fs::path& database_directory_path) {

  auto config_json = read_json_from_disk(local_config_file_path).at("algorithm");
  auto epi_cfg = parse_json_epilink_config(config_json);

  auto record_json = read_json_from_disk(record_file_path).at("fields");
  cout << record_json.dump(2) << endl;
  auto record = parse_json_fields(epi_cfg.fields, record_json);

  map<FieldName, VFieldEntry> db;
  for (auto& f: fs::directory_iterator(database_directory_path)) {
    if (f.path().extension() == ".json") {
      auto db_json = read_json_from_disk(f);
      auto temp_db = parse_json_fields_array(epi_cfg.fields, db_json.at("records"));
      append_to_map_of_vectors(temp_db, db);
    }
  }

  return {epi_cfg, {record, db.size()}, db};
}

EpilinkInput input_test_json() {
  string TestScriptsDir = "../test_scripts/";
  auto dirp = getenv("SEL_TEST_DIR");
  if (dirp) TestScriptsDir = dirp;

  return input_json(
      TestScriptsDir + "configurations/local_init_tuda1.json"s,
      TestScriptsDir + "configurations/validlink.json"s,
      TestScriptsDir + "database"s);
}

} /* END namespace sel::test */

using namespace sel;
using namespace sel::test;

int main(int argc, char *argv[])
{
  bool role_server = false;
  bool only_local = false;
  unsigned int sharing = S_BOOL;
  uint32_t nvals = 1;
  uint32_t nthreads = 1;

  cxxopts::Options options{"test_aby", "Test ABY related components"};
  options.add_options()
    ("S,server", "Run as server. Default to client", cxxopts::value(role_server))
    ("s,sharing", "Boolean sharing to use. 0: GMW, 1: YAO", cxxopts::value(sharing))
    ("n,nvals", "Parallellity", cxxopts::value(nvals))
    ("r,run-both", "Use run_as_both()", cxxopts::value(run_both))
    ("L,local-only", "Only run local calculations on clear values. Doesn't initialize the SecureEpilinker.")
    ("v,verbose", "Set verbosity. May be specified multiple times to log on "
      "info/debug/trace level. Default level is warning.")
    ("h,help", "Print help");
  auto op = options.parse(argc, argv);

  if (op["help"].as<bool>()) {
    cout << options.help() << endl;
    return 0;
  }

  // Logger
  create_terminal_logger();
  switch(op.count("verbose")){
    case 0: spdlog::set_level(spdlog::level::warn); break;
    case 1: spdlog::set_level(spdlog::level::info); break;
    case 2: spdlog::set_level(spdlog::level::debug); break;
    default: spdlog::set_level(spdlog::level::trace); break;
  }

  role = role_server ? SERVER : CLIENT;

  SecureEpilinker::ABYConfig aby_cfg {
    role, (e_sharing)sharing, "127.0.0.1", 5676, nthreads
  };

  const auto in = input_test_json();

  if(!op["local-only"].as<bool>()) {
    SecureEpilinker linker{aby_cfg, in.cfg};
    linker.connect();
    linker.build_circuit(nvals);
    linker.run_setup_phase();

#ifdef DEBUG_SEL_CIRCUIT
    Result res = run(linker, in.client, in.server);
#else
    Result res = (role == CLIENT) ?
      linker.run_as_client(in.client) : linker.run_as_server(in.server);
#endif

    print("Result:\n{}", res);
  }

  print("----- Local Calculations -----\n");
  CircuitConfig cfg32{in.cfg, false, 32};
  auto res_32bit = clear_epilink::calc_integer({in.client, in.server}, cfg32);
  print("32Bit Result:\n{}", res_32bit);

  cfg32.set_ideal_precision();
  auto res_32bit_ideal = clear_epilink::calc_integer({in.client, in.server}, cfg32);
  print("32Bit ideal Result:\n{}", res_32bit_ideal);

  CircuitConfig cfg64{in.cfg, false, 64};
  auto res_64bit = clear_epilink::calc<uint64_t>({in.client, in.server}, cfg64);
  print("64bit Result:\n{}", res_64bit);

  cfg64.set_ideal_precision();
  auto res_64bit_ideal = clear_epilink::calc<uint64_t>({in.client, in.server}, cfg64);
  print("64bit ideal Result:\n{}", res_64bit_ideal);

  auto res_exact = clear_epilink::calc_exact({in.client, in.server}, in.cfg);
  print("Exact Result:\n{}", res_exact);

  return 0;
}
