#include "cxxopts.hpp"
#include "../include/secure_epilinker.h"
#include "../include/util.h"
#include "abycore/aby/abyparty.h"

using namespace sel;
using namespace std;
using Result = SecureEpilinker::Result;

Result test_simple(const SecureEpilinker::ABYConfig& aby_cfg,
    uint32_t nvals) {
  // First test: only one bin field, single byte bitmask
  EpilinkConfig epi_cfg {
    {4.0}, {1.0}, // hw/bin weights
    {}, {}, // hw/bin exchange groups
    8, 0.9, 0.7 // size_bitmask, (tent.) thresholds
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

  auto res = test_simple(aby_cfg, nvals);

  return 0;
}
