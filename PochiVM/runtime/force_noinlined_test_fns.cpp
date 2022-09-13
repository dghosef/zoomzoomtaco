#include "force_noinlined_test_fns.h"

NOINLINE void increment(std::vector<uint64_t>::iterator &it) {
  it++;
}

NOINLINE std::vector<uint64_t>::iterator begin(std::vector<uint64_t> &vec) {
  return vec.begin();
}

NOINLINE std::vector<uint64_t>::iterator end(std::vector<uint64_t> &vec) {
  return vec.end();
}

NOINLINE uint64_t data(std::vector<uint64_t>::iterator &it) {
  return *it;
}

NOINLINE uint64_t it_lt(std::vector<uint64_t>::iterator &it1, std::vector<uint64_t>::iterator &it2) {
  return it1 < it2;
}

void increment_wrapper(std::vector<uint64_t>::iterator &it) {
  increment(it);
}

INLINE std::vector<uint64_t>::iterator begin_wrapper(std::vector<uint64_t> &vec) {
  return begin(vec);
}

INLINE std::vector<uint64_t>::iterator end_wrapper(std::vector<uint64_t> &vec) {
  return end(vec);
}

INLINE uint64_t data_wrapper(std::vector<uint64_t>::iterator &it) {
  return data(it);
}

INLINE bool it_lt_wrapper(std::vector<uint64_t>::iterator &it1, std::vector<uint64_t>::iterator &it2) {
  return it_lt(it1, it2);
}

