#include "pochivm.h" // For all PochiVM APIs
#include "test_util_helper.h"
#include "gtest/gtest.h" // For GoogleTest infrastructure
#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <random>
#include <vector>

class AutoProfiler {
public:
  AutoProfiler(std::string name)
      : m_name(std::move(name)),
        m_beg(std::chrono::high_resolution_clock::now()) {}
  ~AutoProfiler() {
    auto end = std::chrono::high_resolution_clock::now();
    auto dur =
        std::chrono::duration_cast<std::chrono::microseconds>(end - m_beg);
    std::cout << m_name << " : " << dur.count() << " musec" << std::endl;
  }

private:
  std::string m_name;
  std::chrono::time_point<std::chrono::high_resolution_clock> m_beg;
};

using namespace PochiVM; // All PochiVM APIs live in this namespace
// https://stackoverflow.com/questions/21516575/fill-a-vector-with-random-numbers-c
static std::vector<uint64_t> generate_data(size_t size) {
  using value_type = uint64_t;
  // We use static in order to instantiate the random engine
  // and the distribution once only.
  // It may provoke some thread-safety issues.
  static std::uniform_int_distribution<value_type> distribution(
      std::numeric_limits<value_type>::min(),
      std::numeric_limits<value_type>::max());
  static std::default_random_engine generator;

  std::vector<value_type> data(size);
  std::generate(data.begin(), data.end(),
                []() { return distribution(generator); });
  return data;
}

TEST(Bench, SumVector) {
  // Initialize PochiVM global contexts for this thread
  // The contexts are automatically destructed on destruction of these variables
  AutoThreadPochiVMContext apv;
  AutoThreadErrorContext arc;
  AutoThreadLLVMCodegenContext alc;

  // code in snippets below are inserted here
  NewModule("test" /*name*/);
  using FnPrototype = uint64_t (*)(std::vector<uint64_t> *);
  auto [fn, vec] = NewFunction<FnPrototype>("sum_vec");
  auto sum = fn.NewVariable<uint64_t>();
  auto it = fn.NewVariable<std::vector<uint64_t>::iterator>();
  auto vecend = fn.NewVariable<std::vector<uint64_t>::iterator>();
  fn.SetBody(Declare(sum, Literal<uint64_t>(0UL)),
             Declare(vecend, vec->end()),
             For(Declare(it, vec->begin()),
                 it < vecend,
                 it++)
                 .Do(Assign(sum, sum + *it)),
             Return(sum));
  // Validate that the module does not contain errors.
  // We require doing this in test build before JIT'ing the module
  TestAssert(thread_pochiVMContext->m_curModule->Validate());

  // Unfortunately the API for JIT part is not done yet, so we still need some
  // ugly code
  thread_pochiVMContext->m_curModule->EmitIR();


  thread_pochiVMContext->m_curModule->OptimizeIR(3);
  SimpleJIT jit;
  jit.SetModule(thread_pochiVMContext->m_curModule);

  // Retrive the built function, and execute it.
  FnPrototype jitFn = jit.GetFunction<FnPrototype>("sum_vec");
  auto myvec = generate_data(1000000);
  uint64_t result = 0;
  {
    AutoProfiler ap("sum_noinline");
    result = jitFn(&myvec);
  }
  TestAssert(result == std::accumulate(myvec.begin(), myvec.end(), 0UL));
  TestAssert(0);
}
