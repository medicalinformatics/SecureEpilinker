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

Result test_simple(const SecureEpilinker::ABYConfig& aby_cfg,
    uint32_t nvals) {
  // First test: only one bin field, single byte bitmask
  EpilinkConfig epi_cfg {
    {
      {BM, {4.0}},
      {BIN, {1.0}}
    }, // bm/bin weights
    {{BM, {}}, {BIN, {}}}, // bm/bin exchange groups
    8, Threshold, TThreshold // size_bitmask, (tent.) thresholds
  };

  EpilinkClientInput in_client {
    {{0x33}}, {0xdeadbeef}, // hw/bin record
    {false}, {false}, // hw/bin empty
    nvals // nvals
  };

  EpilinkServerInput in_server {
    {vector<Bitmask>(nvals, {0x33})}, {vector<CircUnit>(nvals, 0xdeadbeef)}, // hw/bin database
    {vector<bool>(nvals, false)}, {vector<bool>(nvals, false)}, // hw/bin empty
  };

  SecureEpilinker linker{aby_cfg, epi_cfg};

  linker.build_circuit(nvals);
  linker.run_setup_phase();

  Result res;
  if (aby_cfg.role == SERVER) {
    res = linker.run_as_server(in_server);
  } else {
    res = linker.run_as_client(in_client);
  }
  linker.reset();

  return res;
}

Result test_exchange_grp(const SecureEpilinker::ABYConfig& aby_cfg,
    uint32_t nvals) {
  // First test: only one bin field, single byte bitmask
  EpilinkConfig epi_cfg {
    {
      {BM, {4.0, 1.0}},
      {BIN, {2.0, 1.0}}
    }, // bm/bin weights
    {{BM, {{0,1}}}, {BIN, {}}}, // bm/bin exchange groups
    8, Threshold, TThreshold // size_bitmask, (tent.) thresholds
  };

  EpilinkClientInput in_client {
    {{0x33},{0x43}}, // hw records
    {0xdeadbeef, 0xdecea5ed}, // bin records
    {false, false}, {false, false}, // hw/bin empty
    nvals // nvals
  };

  EpilinkServerInput in_server {
    {vector<Bitmask>(nvals, {0x44}), vector<Bitmask>(nvals, {0x33})}, // hw db
    {vector<CircUnit>(nvals, 0xdeadbeef), vector<CircUnit>(nvals, 0xdecea5ed)}, // bin database
    {vector<bool>(nvals, false), vector<bool>(nvals, false)},// hw empty
    {vector<bool>(nvals, false), vector<bool>(nvals, false)} // bin empty
  };

  SecureEpilinker linker{aby_cfg, epi_cfg};

  linker.build_circuit(nvals);
  linker.run_setup_phase();

  Result res;
  /*
  if (aby_cfg.role == SERVER) {
    res = linker.run_as_server(in_server);
  } else {
    res = linker.run_as_client(in_client);
  }
  */
  res = linker.run_as_both(in_client, in_server);
  linker.reset();

  return res;
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
    ("h,help", "Print help");
  auto op = options.parse(argc, argv);

  if (op.count("help") != 0) {
    cout << options.help() << endl;
    return 0;
  }

  e_role role = role_server ? SERVER : CLIENT;

  SecureEpilinker::ABYConfig aby_cfg {
    role, (e_sharing)sharing, "127.0.0.1", 5676, nthreads
  };

  //auto res = test_simple(aby_cfg, nvals);
  Result res = test_exchange_grp(aby_cfg, nvals);

  print_result(res);

  return 0;
}
