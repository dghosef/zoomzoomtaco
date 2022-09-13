#include <cstdint>
#include <vector>
#define NOINLINE // __attribute__((noinline))
#define INLINE
INLINE void increment_wrapper(std::vector<uint64_t>::iterator &it);
INLINE std::vector<uint64_t>::iterator begin_wrapper(std::vector<uint64_t> &vec);
INLINE std::vector<uint64_t>::iterator end_wrapper(std::vector<uint64_t> &vec);
INLINE uint64_t data_wrapper(std::vector<uint64_t>::iterator &it);
INLINE bool it_lt_wrapper(std::vector<uint64_t>::iterator &it1, std::vector<uint64_t>::iterator &it2);
