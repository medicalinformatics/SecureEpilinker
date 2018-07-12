#include "fmt/format.h"
#include "../include/util.h"

using namespace std;

namespace sel {

void test_format_vector() {
  vector<string>v({"ab", "cd", "ef"});
  assert (fmt::format("{}", v) == "[ab, cd, ef]");
}

void test_vector_bool_to_bitmask() {
  // big endian |00 11 00 00|
  vector<bool> vb{false, false, true, true, false, false, false, false};
  vector<uint8_t> bm = vector_bool_to_bitmask(vb);
  assert (bm.size() == 1);
  assert (bm[0] == 0x0c);

  // big endian |00 11 00 00 | 1
  vb.emplace_back(true);
  bm = vector_bool_to_bitmask(vb);
  assert (bm.size() == 2);
  assert (bm[1] == 1);
}

void test_ceil_log2() {
  for (int i = 0; i != sizeof(unsigned long long)*8-1; ++i) {
    unsigned long long x = (1ULL << i);
    assert (sel::ceil_log2(x) == i);
    assert (sel::ceil_log2(x+1) == i+1);
  }
}

void test_map() {
  map<int, double> nums{{0,3.4}, {1,4.1}, {2,16.9}};
  auto numsi = transform_map(nums, [](double x){
      return static_cast<int>(x);
      });

  map<int, int> numst{{0,3}, {1,4}, {2,16}};
  assert (numsi ==  numst);
}

void test_map_str_double_to_vector_double() {
  map<string,double> mw{{"f1", 2.3},{"f2",0.8},{"f3", 0.6543}};
  vector<double> vw{transform_map_vec(mw,[&mw](pair<string,double> t){return t.second;})};
  assert (vw.size() == mw.size());
}

} // namespace sel

using namespace sel;

int main(int argc, char *argv[])
{
  test_vector_bool_to_bitmask();
  test_ceil_log2();
  test_map();
  test_format_vector();
  return 0;
}
