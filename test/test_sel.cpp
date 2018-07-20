#include <random>
#include "cxxopts.hpp"
#include "fmt/format.h"
#include "../include/util.h"
#include "../include/secure_epilinker.h"
#include "../include/clear_epilinker.h"
#include "abycore/aby/abyparty.h"

using namespace sel;
using namespace std;
using Result = SecureEpilinker::Result;
using fmt::print;

constexpr auto BIN = FieldComparator::BINARY;
constexpr auto BM = FieldComparator::DICE;
constexpr double Threshold = 0.9;
constexpr double TThreshold = 0.7;

struct EpilinkInput {
  EpilinkConfig cfg;
  EpilinkClientInput client;
  EpilinkServerInput server;
};

ML_Field f_int1 (
  //name, f, e, comparator, type, bitsize
  "int_1", 1.0,
  FieldComparator::BINARY, FieldType::INTEGER, 32
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

EpilinkConfig dkfz_cfg {
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

// Whether to use run_as_{client/server}() or run_as_both()
bool run_both{false};
e_role role;

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

EpilinkInput input_exchange_grp(uint32_t nvals) {
  // First test: only one bin field, single byte bitmask
  EpilinkConfig epi_cfg {
    {
      {"int_1", f_int1},
      {"int_2", f_int2},
      {"bm_1", f_bm1},
      {"bm_2", f_bm2},
    }, // fields
    {{"int_1", "int_2"}}, // exchange groups
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
      {"bm_1", vector<FieldEntry>(nvals, Bitmask{0x33})},
      {"bm_2", vector<FieldEntry>(nvals, Bitmask{0x42})}, // 1-bit mismatch
      {"int_1", vector<FieldEntry>(nvals, data_int_2)},
      {"int_2", vector<FieldEntry>(nvals, data_int_1)}
    } // records
  };

  return {epi_cfg, in_client, in_server};
}

mt19937 gen(73);
uniform_int_distribution<> rndbyte(0, 0xff);

Bitmask random_bm(size_t bitsize, int density_shift) {
  Bitmask bm(bitbytes(bitsize));
  for(auto& b : bm) {
    b = rndbyte(gen);
    for (int i = 0; i != abs(density_shift); ++i) {
      if (density_shift > 0)  b |= rndbyte(gen);
      else if (density_shift < 0)  b &= rndbyte(gen);
    }
  }
  // set most significant bits to 0 if bitsize%8
  if (bitsize%8) bm.back() >>= (8-bitsize%8);
  return bm;
}

/**
 * Generates random input for the given EpilinkConfig.
 *
 * bm_density_shift controls the density of randomly set bits in the bitmasks:
 *   = 0 - equal number of 1s and 0s
 *   > 0 - more 1s than 0s
 *   < 0 - less 1s than 0s
 *
 * bin_match_prob gives the random probability with which a database bin field
 * is set to the corresponding client record entry. Otherwise the probability of
 * a match for randomly generated binary fields would diminishingly low.
 */
EpilinkInput input_random(const EpilinkConfig& cfg,
    uint32_t nvals, int bm_density_shift = 0, double bin_match_prob = .5) {
  EpilinkClientInput in_client {
    transform_map(dkfz_cfg.fields, [&bm_density_shift](const ML_Field& f)
      -> FieldEntry {
        return random_bm(f.bitsize,
            (f.comparator == FieldComparator::DICE) ? bm_density_shift : 0);
      }),
      nvals
  };

  bernoulli_distribution rndmatch(bin_match_prob);

  EpilinkServerInput in_server {
    transform_map(dkfz_cfg.fields,
      [&nvals, &bm_density_shift, &rndmatch, &in_client](const ML_Field& f)
      -> VFieldEntry {
        VFieldEntry ve;
        ve.reserve(nvals);
        for (auto i = 0; i < nvals; ++i) {
          if (f.comparator == FieldComparator::DICE) {
            ve.emplace_back(random_bm(f.bitsize, bm_density_shift));
          } else if (rndmatch(gen)) {
            ve.emplace_back(in_client.record.at(f.name));
          } else {
            ve.emplace_back(random_bm(f.bitsize, 0));
          }
        }
        return ve;
      })
  };

  return {cfg, in_client, in_server};
}

EpilinkInput input_empty(uint32_t nvals) {
  // First test: only one bin field, single byte bitmask
  EpilinkConfig epi_cfg {
    {
      {"bm_1", f_bm1},
      {"bm_2", f_bm2},
    }, // fields
    {}, // exchange groups
    Threshold, TThreshold // size_bitmask, (tent.) thresholds
  };

  EpilinkClientInput in_client {
    {
      {"bm_1", Bitmask{0x33}},
      {"bm_2", Bitmask{0x44}},
    }, // record
    2 // nvals
  };

  EpilinkServerInput in_server {
    {
      {"bm_1", { nullopt, Bitmask{0x31} }}, // 1-bit mismatch for #1
      {"bm_2", { Bitmask{0x43}, nullopt }}, // 2-bit mismatch for #0
    } // records
  };

  return {epi_cfg, in_client, in_server};
}

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
    ("h,help", "Print help");
  auto op = options.parse(argc, argv);

  if (op["help"].as<bool>()) {
    cout << options.help() << endl;
    return 0;
  }

  role = role_server ? SERVER : CLIENT;

  SecureEpilinker::ABYConfig aby_cfg {
    role, (e_sharing)sharing, "127.0.0.1", 5676, nthreads
  };

  const auto in = input_random(dkfz_cfg, nvals);

  if(!op["local-only"].as<bool>()) {
    SecureEpilinker linker{aby_cfg, in.cfg};
    linker.build_circuit(nvals);
    linker.run_setup_phase();

    Result res = run(linker, in.client, in.server);

    print("Result:\n{}", res);
  }

  auto res_local = clear_epilink::calc_integer({in.client, in.server}, in.cfg);
  print("Local Result:\n{}", res_local);

  auto res_exact = clear_epilink::calc_exact({in.client, in.server}, in.cfg);
  print("Exact Result:\n{}", res_exact);

  return 0;
}
