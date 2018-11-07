#include "../include/util.h"
#include "../include/math.h"
#include "../include/aby/Share.h"
#include "../include/aby/gadgets.h"
#include "../include/aby/quotient_folder.hpp"
#include "abycore/aby/abyparty.h"
#include "abycore/sharing/sharing.h"
#include "cxxopts.hpp"
#include <numeric>
#include <algorithm>
#include <random>
#include <fmt/format.h>
using fmt::print;

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
  BooleanCircuit* cc;
  ArithmeticCircuit* ac;
  bool zeropad;
  const B2AConverter to_arith_closure;
  const A2BConverter to_bool_closure;
  mt19937 gen;

  ABYTester(e_role role, e_sharing sharing, uint32_t nvals, uint32_t bitlen, uint32_t nthreads, bool _zeropad, uint_fast32_t random_seed) :
    role{role}, bitlen{bitlen}, nvals{nvals}, party{role, "127.0.0.1", 5676, LT, bitlen, nthreads},
    bc{dynamic_cast<BooleanCircuit*>(party.GetSharings()[sharing]->GetCircuitBuildRoutine())},
    cc{dynamic_cast<BooleanCircuit*>(party.GetSharings()[(sharing==S_YAO)?S_BOOL:S_YAO]->GetCircuitBuildRoutine())},
    ac{dynamic_cast<ArithmeticCircuit*>(party.GetSharings()[S_ARITH]->GetCircuitBuildRoutine())},
    zeropad{_zeropad},
    to_arith_closure{[this](auto x){return to_arith(x);}},
    to_bool_closure{[this](auto x){return to_bool(x);}},
    gen(random_seed)
  {
    cout << "Testing ABY with role: " << get_role_name(role) <<
     " with sharing: " << get_sharing_name(sharing) << " nvals: " << nvals <<
     " bitlen: " << bitlen << endl;
    party.ConnectAndBaseOTs();
  }

  // Dynamic converters, dependent on main bool sharing
  BoolShare to_bool(const ArithShare& s) {
    return (bc->GetContext() == S_YAO) ? a2y(bc, s) : a2b(bc, cc, s);
  }
  ArithShare to_arith(const BoolShare& s_) {
    BoolShare s = zeropad ? s_.zeropad(bitlen) : s_; // fix for aby issue #46
    return (bc->GetContext() == S_YAO) ? y2a(ac, cc, s) : b2a(ac, s);
  }

  vector<uint64_t> make_random_vector(size_t bitrange) {
    uniform_int_distribution<uint64_t> random_value(0, (1ull << bitrange)-1);
    vector<uint64_t> v(nvals);
    for (auto& x : v) {
      x = random_value(gen);
    }
    return v;
  }

  void test_bm_input() {
    uint32_t _bitlen = bitlen;
    auto bytelen = bitbytes(_bitlen);
    Bitmask data(bytelen);
    iota(data.begin(), data.end(), 0x42);

    BoolShare in, in2;
    if (role==CLIENT) {
      in = BoolShare{bc, repeat_vec(data, nvals).data(), _bitlen, CLIENT, nvals};
      in2 = BoolShare{bc, _bitlen, nvals};
    } else {
      in2 = BoolShare{bc, repeat_vec(data, nvals).data(), _bitlen, SERVER, nvals};
      in = BoolShare{bc, _bitlen, nvals};
    }

    cout << hex;
    print_share(in, "in");
    print_share(in2, "in2");

    party.ExecCircuit();
  }

  void test_reinterpret() {
    ArithShare a{ac, vector<uint32_t>(bitlen, 0xdeadbeef).data(), bitlen, SERVER, bitlen};
    ArithShare azero{ac, vector<uint32_t>(bitlen, 0).data(), bitlen, SERVER, bitlen};
    print_share(a, "a");

    BoolShare b{bc, new boolshare(a.get()->get_wires(), bc)};
    BoolShare bzero{bc, new boolshare(azero.get()->get_wires(), bc)};

    BoolShare btrue{bc, 1u, bitlen, CLIENT};
    BoolShare bmux = btrue.mux(b, bzero);
    print_share(bmux, "bmux");
    //print_share(amux, "amux");

    //BoolShare b = reinterpret_share(a, bc);
    //print_share(b, "b");

    party.ExecCircuit();
  }

  void test_conversion() {
    vector<uint32_t> data(max(nvals, 2u));
    iota(data.begin(), data.end(), 0);
    size_t data_bitlen = sel::ceil_log2_min1(nvals);
    BoolShare in(bc, data.data(), data_bitlen, SERVER, nvals);
    BoolShare in2(bc, data.data(), data_bitlen, SERVER, 2);
    ArithShare ain = to_arith(in);
    ArithShare ain2 = to_arith(in2);

    BoolShare bain = to_bool(ain);
    ArithShare abain = to_arith(bain);

    print_share(in, "bool in");
    print_share(in2, "bool in2");
    print_share(ain, "arithmetic in");
    print_share(ain2, "arithmetic in2");

    print_share(bain, "b(a(in))");
    print_share(abain, "a(b(a(in)))");

    party.ExecCircuit();
  }

  void test_deterministic_aby_chaos() {
    size_t bbits = 8;
    vector<uint8_t> bdata(bitbytes(bbits) * nvals, 0);
    iota(bdata.begin(), bdata.end(), 0);
    BoolShare bin(bc, bdata.data(), bbits, CLIENT, nvals);
    auto a_bin = to_arith(bin);
    auto band = (bin & bin);
    auto a_band = to_arith(band);

    print_share(band, "b & b");
    print_share(a_bin, "arith(b)");
    print_share(a_band, "arith(b & b)");

    party.ExecCircuit();
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

    print("a*c = {}", out_ac.get_clear_value_vec());
  }

  void test_max_bits() {
    size_t bytes = 64;
    vector<uint8_t> data(bytes, 0xc3u);
    vector<uint8_t> data2(bytes, 0x95u);
    print("data: {}", data);
    print("data2: {}", data);

    BoolShare in(bc, data.data(), bytes*8-1, CLIENT, 1);
    BoolShare in2(bc, data2.data(), bytes*8-1, CLIENT, 1);
    print_share(in, "in");
    print_share(in2, "in2");

    BoolShare x = in & in2;
    print_share(x, "in & in2");

    x = hammingweight(x);
    print_share(x, "HW");

    party.ExecCircuit();
  }

  void test_hw() {
    using datatype = uint32_t;
    vector<datatype> data = {0xdeadbeef, 0x33333333, 0x0};
    vector<datatype> data2 = {0xdeadbee0, 0x03333333, 0xffffffff};
    size_t _bitlen = sizeof(datatype)*8;

    BoolShare in;
    if (role == CLIENT) {
      in = BoolShare(bc, data.data(), _bitlen, CLIENT, data.size());
    } else {
      in = BoolShare(bc, _bitlen, data.size());
    }
    cout << hex;
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

  void test_max_quotient() {
    ArithQuotient a = {ArithShare{ac, 24u, bitlen, SERVER},
      ArithShare{ac, 5u, bitlen, CLIENT}};
    ArithQuotient b = {ArithShare{ac, 16u, bitlen, SERVER},
      ArithShare{ac, 13u, bitlen, CLIENT}};
    ArithQuotient c = {ArithShare{ac, 3u, bitlen, SERVER},
      ArithShare{ac, 155u, bitlen, CLIENT}};

    ArithQuotient maxab = max({a, b, c}, to_bool_closure, to_arith_closure);

    print_share(a, "a");
    print_share(b, "b");
    print_share(c, "c");
    print_share(maxab, "maxab");

    party.ExecCircuit();
  }

  template <class MultShare>
  std::conditional_t<std::is_same_v<MultShare, ArithShare>,
    ArithmeticCircuit, BooleanCircuit>* circuit() {
      if constexpr (std::is_same_v<MultShare, ArithShare>) {
        return ac;
      } else {
        return bc;
      }
  }

  template <class MultShare>
  void test_quotient_folder() {
    auto circ = circuit<MultShare>();
    size_t num_bits = llround(2*((double)(bitlen)/3));
    size_t den_bits = llround((double)(bitlen)/3);
    assert (num_bits + den_bits == bitlen);
    auto data_num = make_random_vector(num_bits);
    auto data_den = make_random_vector(den_bits);
    print("numerators: {}\ndenominators: {}\n", data_num, data_den);

    uint64_t max_num = 0, max_den = 1;
    size_t max_idx = numeric_limits<size_t>::max();
    for (size_t i = 0; i < nvals; ++i) {
      auto num = data_num[i], den = data_den[i];
      if (den == 0) continue;
      if ( (num * max_den > max_num * den)
          or ( (num * max_den == max_num * den) and (den > max_den) )
         ) {
        max_den = den, max_num = num;
        max_idx = i;
      }
    }
    fmt::print("Maximum num: {}, den: {}, index: {}\n", max_num, max_den, max_idx);

    Quotient<MultShare> inq = {
      {circ, data_num.data(), bitlen, SERVER, nvals},
      {circ, data_den.data(), bitlen, CLIENT, nvals}
    };
    print_share(inq.num, "num");
    print_share(inq.den, "den");

    vector<BoolShare> targets = {ascending_numbers_constant(bc, nvals)};
    //QuotientSelector<ArithShare> op_max = make_max_selector(to_bool_closure);
    //split_select_quotient_target(inq, targets, op_max, to_arith_closure);

    using QF = QuotientFolder<MultShare>;

    QF folder(move(inq), move(targets), QF::FoldOp::MAX_TIE);
    if constexpr (std::is_same_v<MultShare, ArithShare>) {
      folder.set_converters_and_den_bits(&to_bool_closure, &to_arith_closure, den_bits);
    }
    auto res = folder.fold();

    print_share(res.get_selector().num, "max num");
    print_share(res.get_selector().den, "max den");
    print_share(res.get_targets()[0], "index of max");

    party.ExecCircuit();
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
  bool zeropad = false;
  uint_fast32_t random_seed = 73;

  cxxopts::Options options{"test_aby", "Test ABY related components"};
  options.add_options()
    ("S,server", "Run as server. Default to client", cxxopts::value(role_server))
    ("s,sharing", "Boolean sharing to use. 0: GMW, 1: YAO", cxxopts::value(sharing))
    ("n,nvals", "Parallellity", cxxopts::value(nvals))
    ("b,bitlen", "Bitlength", cxxopts::value(bitlen))
    ("z,zeropad", "Enable zeropadding before B2A conversion", cxxopts::value(zeropad))
    ("R,random-seed", "Random generator seed", cxxopts::value(random_seed))
    ("h,help", "Print help");
  auto op = options.parse(argc, argv);

  if (op.count("help") != 0) {
    cout << options.help() << endl;
    return 0;
  }

  e_role role = role_server ? SERVER : CLIENT;

  ABYTester tester{role, (e_sharing)sharing, nvals, bitlen, nthreads, zeropad, random_seed};

  //tester.test_split_select_target();
  //tester.test_add();
  //tester.test_mult_const();
  //tester.test_hw();
  //tester.test_max_bits();
  //tester.test_conversion();
  //tester.test_reinterpret();
  //tester.test_split_accumulate();
  tester.test_quotient_folder<BoolShare>();
  //tester.test_max_quotient();
  //tester.test_bm_input();
  //tester.test_deterministic_aby_chaos();

  return 0;
}
