#include "../sel/util.h"

using namespace std;

namespace sel {

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

void test_util() {
  test_vector_bool_to_bitmask();
}

} // namespace sel
