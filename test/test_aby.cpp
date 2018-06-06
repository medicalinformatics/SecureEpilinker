#include <numeric>
#include "cxxopts.hpp"
#include "../include/util.h"
#include "../include/aby/Share.h"
#include "../include/aby/gadgets.h"
#include "abycore/aby/abyparty.h"

using namespace std;
using namespace sel;

BoolShare op_max(const BoolShare& a, const BoolShare& b) {
  return (a > b).mux(a, b);
}

BoolShare op_gt(const BoolShare& a, const BoolShare& b) {
  return a > b;
}

BoolShare op_add(const BoolShare& a, const BoolShare& b) {
  return a + b;
}

struct ABYTester {
  e_role role;
  uint32_t bitlen;
  uint32_t nvals;
  ABYParty party;
  BooleanCircuit* bc;
  ArithmeticCircuit* ac;

  ABYTester(e_role role, e_sharing sharing, uint32_t nvals, uint32_t bitlen, uint32_t nthreads) :
    role{role}, bitlen{bitlen}, nvals{nvals}, party{role, "127.0.0.1", 5676, LT, bitlen, nthreads},
    bc{dynamic_cast<BooleanCircuit*>(party.GetSharings()[sharing]->GetCircuitBuildRoutine())},
    ac{dynamic_cast<ArithmeticCircuit*>(party.GetSharings()[S_ARITH]->GetCircuitBuildRoutine())}
  {
    cout << "Testing ABY with role: " << get_role_name(role) <<
     " with sharing: " << get_sharing_name(sharing) << " nvals: " << nvals <<
     " bitlen: " << bitlen << endl;
  }

  void test_mult_const() {
    vector<uint32_t> vin(nvals);
    iota(vin.begin(), vin.end(), 0);
    ArithShare a{ac, vin.data(), bitlen, SERVER, nvals};
    ArithShare c = constant_simd(ac, 3, bitlen, nvals);
    print_share(a, "a");
    print_share(c, "c");
    ArithShare ac = a * c;
    print_share(ac, "a*c");

    auto out_ac = out(ac, ALL);

    party.ExecCircuit();

    cout << "a*c = " << out_ac.get_clear_value_vec() << endl;
  }

  void test_hw() {
    using datatype = uint64_t;
    vector<datatype> data = {0xdeadbeef, 0x33333333};
    vector<datatype> data2 = {0xdeadbee0, 0x03333333};
    size_t _bitlen = sizeof(datatype)*8;

    BoolShare in;
    if (role == CLIENT) {
      in = BoolShare(bc, data.data(), _bitlen, CLIENT, data.size());
    } else {
      in = BoolShare(bc, _bitlen, data.size());
    }
    print_share(in, "in");

    BoolShare in2(bc, data2.data(), _bitlen, SERVER, data2.size());
    print_share(in2, "in2");

    BoolShare bAND = in & in2;
    print_share(bAND, "in & in2");

    in = hammingweight(bAND);
    print_share(in, "hw");

    party.ExecCircuit();
  }

  void test_split_accumulate() {
    vector<uint32_t> vin{2, 40, 67, 119, 2839};
    vector<uint32_t> xin(5,100);

    BoolShare a_in, b_in;
    if (role == SERVER) {
      a_in = BoolShare{bc, bitlen, 5};
      b_in = BoolShare{bc, vin.data(), bitlen, SERVER, 5};
    } else {
      a_in = BoolShare{bc, xin.data(), bitlen, CLIENT, 5};
      b_in = BoolShare{bc, bitlen, 5};
    }

    //b_in = constant_simd(bc, 4, 32, 5);

    print_share(a_in, "a_in");
    print_share(b_in, "b_in");

    BoolShare s_add = a_in + b_in;
    print_share(s_add, "s_add");

    BoolShare s_max = split_accumulate(s_add, op_max);
    BoolShare s_sum = split_accumulate(s_add, op_add);

    print_share(s_max, "max");
    print_share(s_sum, "sum");

    OutShare out_max = out(s_max, ALL);
    OutShare out_sum = out(s_sum, ALL);

    party.ExecCircuit();

    cout << "max: " << out_max.get_clear_value<uint32_t>()
      << " sum: " << out_sum.get_clear_value<uint32_t>()
      << endl;
  }

  void test_split_select_target() {
    vector<uint32_t> vin{2, 40, 1034, 67, 678};
    vector<uint32_t> xin{1,2,3,4,5};

    BoolShare a_in{bc, vin.data(), bitlen, SERVER, 5};
    BoolShare b_in{bc, xin.data(), bitlen, CLIENT, 5};

    print_share(a_in, "a_in");
    print_share(b_in, "b_in");

    split_select_target(a_in, b_in, op_gt);

    print_share(a_in, "max a");
    print_share(b_in, "selected b");

    OutShare out_a = out(a_in, ALL);
    OutShare out_b = out(b_in, ALL);

    party.ExecCircuit();

    cout << "max a: " << out_a.get_clear_value<uint32_t>()
      << " selected b: " << out_b.get_clear_value<uint32_t>()
      << endl;
  }

  void test_add() {
    constexpr uint32_t _bitlen = 8;
    BoolShare a = (role==SERVER) ? BoolShare{bc, _bitlen} : BoolShare{bc, 43u, _bitlen, CLIENT};
    BoolShare b = (role==CLIENT) ? BoolShare{bc, _bitlen} : BoolShare{bc, 67u, _bitlen, SERVER};

    a = a.repeat(10);
    b = b.repeat(10);

    print_share(a, "a");
    print_share(b, "b");

    BoolShare ab = a + b;
    print_share(ab, "a+b");
    OutShare out_ab = out(ab, ALL);

    party.ExecCircuit();

    cout << "a+b: " << out_ab.get_clear_value<uint32_t>() << endl;
  }
};

int main(int argc, char *argv[])
{
  bool role_server = false;
  unsigned int sharing = S_BOOL;
  uint32_t nvals = 1;
  uint32_t bitlen = 32;
  uint32_t nthreads = 1;

  cxxopts::Options options{"test_aby", "Test ABY related components"};
  options.add_options()
    ("S,server", "Run as server. Default to client", cxxopts::value(role_server))
    ("s,sharing", "Boolean sharing to use. 0: GMW, 1: YAO", cxxopts::value(sharing))
    ("n,nvals", "Parallellity", cxxopts::value(nvals))
    ("b,bitlen", "Bitlength", cxxopts::value(bitlen))
    ("h,help", "Print help");
  auto op = options.parse(argc, argv);

  if (op.count("help") != 0) {
    cout << options.help() << endl;
    return 0;
  }

  e_role role = role_server ? SERVER : CLIENT;

  ABYTester tester{role, (e_sharing)sharing, nvals, bitlen, nthreads};

  //tester.test_split_select_target();
  //tester.test_add();
  //tester.test_mult_const();
  tester.test_hw();

  return 0;
}
