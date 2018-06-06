#include "cxxopts.hpp"
#include "../include/secure_epilinker.h"
#include "abycore/aby/abyparty.h"

using namespace sel;
using namespace std;

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

  // First test: only one bin field, single byte bitmask
  EpilinkConfig epi_cfg {
    {2.0}, {1.0}, // hw/bin weights
    {}, {}, // hw/bin exchange groups
    8, 0.9, 0.7 // size_bitmask, (tent.) thresholds
  };

  EpilinkClientInput in_client {
    {{0x33}}, {0xdeadbeef}, // hw/bin record
    {false}, {false}, // hw/bin empty
    1 // nvals
  };

  //EpilinkServerInput in_server {
    //{ {{0x33}, {0x44}} }, {{0xdeadbeef, 0xabbaabba}}, // hw/bin database
    //{{false, false}}, {{false, false}}, // hw/bin empty
  //};
  EpilinkServerInput in_server {
    { {{0x33}} }, {{0xdeadbeef}}, // hw/bin database
    {{false}}, {{false}}, // hw/bin empty
  };

  SecureEpilinker::ABYConfig aby_cfg {
    role, (e_sharing)sharing, "127.0.0.1", 5676, nthreads
  };

  SecureEpilinker linker{aby_cfg, epi_cfg};

  linker.build_circuit(nvals);
  linker.run_setup_phase();

  if (role == SERVER) {
    linker.run_as_server(in_server);
  } else {
    linker.run_as_client(in_client);
  }

  return 0;
}
