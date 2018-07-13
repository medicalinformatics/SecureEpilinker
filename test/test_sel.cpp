#include "cxxopts.hpp"
#include "../include/util.h"
#include "../include/secure_epilinker.h"
#include "abycore/aby/abyparty.h"

using namespace sel;
using namespace std;
using Result = SecureEpilinker::Result;
using fmt::print;

constexpr auto BIN = FieldComparator::BINARY;
constexpr auto BM = FieldComparator::NGRAM;
constexpr double Threshold = 0.9;
constexpr double TThreshold = 0.7;

ML_Field f_int1 (
  //name, f, e, comparator, type, bitsize
  "int_1", 1.0,
  FieldComparator::BINARY, FieldType::INTEGER, 32
);

Bitmask data_int_1 {Bitmask{0x33, 0, 0, 0}};

ML_Field f_int2 (
  //name, f, e, comparator, type, bitsize
  "int_2", 3.0,
  FieldComparator::BINARY, FieldType::INTEGER, 32
);

ML_Field f_bm1 (
  //name, f, e, comparator, type, bitsize
  "bm_1", 2.0,
  FieldComparator::NGRAM, FieldType::BITMASK, 8
);

ML_Field f_bm2 (
  //name, f, e, comparator, type, bitsize
  "bm_2", 4.0,
  FieldComparator::NGRAM, FieldType::BITMASK, 8
);

// Whether to use run_as_{client/server}() or run_as_both()
bool run_both{false};
e_role role;

Result run(SecureEpilinker& linker,
    const EpilinkClientInput& in_client, const EpilinkServerInput& in_server) {
  print("Calling run_as_{}()", run_both ? "both" : ((role==CLIENT) ? "client" : "server"));
  if (!run_both) {
    return (role==CLIENT) ?
      linker.run_as_client(in_client) : linker.run_as_server(in_server);
  } else {
    return linker.run_as_both(in_client, in_server);
  }

}

Result test_simple(const SecureEpilinker::ABYConfig& aby_cfg,
    uint32_t nvals) {
  // First test: only one bin field, single byte integer
  EpilinkConfig epi_cfg {
    { {"int_1", f_int1}, }, // fields
    {}, // exchange groups
    8, Threshold, TThreshold // size_bitmask, (tent.) thresholds
  };

  Bitmask data_int_zero(4, 0);

  EpilinkClientInput in_client {
    { {"int_1", (role==CLIENT) ? data_int_1 : data_int_zero} }, // record
    nvals // nvals
  };

  EpilinkServerInput in_server {
    { {"int_1", vector<FieldEntry>(nvals, (role==SERVER) ? Bitmask{0x33,0,0,0} : data_int_zero)} } // records
  };

  SecureEpilinker linker{aby_cfg, epi_cfg};

  linker.build_circuit(nvals);
  linker.run_setup_phase();

  return run(linker, in_client, in_server);
}

Result test_exchange_grp(const SecureEpilinker::ABYConfig& aby_cfg,
    uint32_t nvals) {
  // First test: only one bin field, single byte bitmask
  EpilinkConfig epi_cfg {
    {
      {"int_1", f_int1},
      {"int_2", f_int2},
      {"bm_1", f_bm1},
      {"bm_2", f_bm2},
    }, // fields
    {{"int_1", "int_2"}}, // exchange groups
    8, Threshold, TThreshold // size_bitmask, (tent.) thresholds
  };

  EpilinkClientInput in_client {
    {
      {"bm_1", Bitmask{0x33, 0, 0, 0}},
      {"bm_2", Bitmask{0x43, 0, 0, 0}},
      {"int_1", Bitmask{0xde, 0xad, 0xbe, 0xef}},
      {"int_2", Bitmask{0xde, 0xce, 0xa5, 0xed}}
    }, // record
    nvals // nvals
  };

  EpilinkServerInput in_server {
    {
      {"bm_1", vector<FieldEntry>(nvals, Bitmask{0x33, 0, 0, 0})},
      {"bm_2", vector<FieldEntry>(nvals, Bitmask{0x43, 0, 0, 0})},
      {"int_1", vector<FieldEntry>(nvals, Bitmask{0xde, 0xad, 0xbe, 0xef})},
      {"int_2", vector<FieldEntry>(nvals, Bitmask{0xde, 0xce, 0xa5, 0xed})}
    } // records
  };

  SecureEpilinker linker{aby_cfg, epi_cfg};

  linker.build_circuit(nvals);
  linker.run_setup_phase();

  return run(linker, in_client, in_server);
}

void print_result(Result result) {
  print("Result:\n{}", result);
}

int main(int argc, char *argv[])
{
  bool role_server = false;
  unsigned int sharing = S_BOOL;
  uint32_t nvals = 1;
  uint32_t nthreads = 1;

  cxxopts::Options options{"test_aby", "Test ABY related components"};
  options.add_options()
    ("S,server", "Run as server. Default to client", cxxopts::value(role_server))
    ("s,sharing", "Boolean sharing to use. 0: GMW, 1: YAO", cxxopts::value(sharing))
    ("n,nvals", "Parallellity", cxxopts::value(nvals))
    ("r,run-both", "Use run_as_both()", cxxopts::value(run_both))
    ("h,help", "Print help");
  auto op = options.parse(argc, argv);

  if (op.count("help") != 0) {
    cout << options.help() << endl;
    return 0;
  }

  role = role_server ? SERVER : CLIENT;

  SecureEpilinker::ABYConfig aby_cfg {
    role, (e_sharing)sharing, "127.0.0.1", 5676, nthreads
  };

  Result res = test_simple(aby_cfg, nvals);
  //Result res = test_exchange_grp(aby_cfg, nvals);

  print_result(res);

  return 0;
}
