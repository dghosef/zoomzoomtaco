#pragma once
#include "pochivm.h"
#include "taco.h"
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
int new_int() {
  static int n = 0;
  return n++;
}
char *malloc_str(std::string cppstr) {
  const char *str = cppstr.c_str();
  char *res = (char *)malloc(strlen(str) + 1);
  strcpy(res, str);
  return res;
}
namespace taco {
bool should_use_pochi_codegen(std::string fnName);
void set_pochi_state(bool usePochi);
bool get_pochi_state();
} // namespace taco
using namespace taco;
class AutoProfiler {
public:
  AutoProfiler(std::string name)
      : m_name(std::move(name)),
        m_beg(std::chrono::high_resolution_clock::now()) {}
  ~AutoProfiler() {
    auto end = std::chrono::high_resolution_clock::now();
    auto dur =
        std::chrono::duration_cast<std::chrono::microseconds>(end - m_beg);
    std::cout << (get_pochi_state() ? "PochiVM: " : "GCC: ");
    std::cout << m_name << " : " << dur.count() << " musec" << std::endl;
  }

private:
  std::string m_name;
  std::chrono::time_point<std::chrono::high_resolution_clock> m_beg;
};
template <typename T> void fn_with_and_without_pochi(T fn, std::string fnName) {
  std::cout << "--------------testing " << fnName << "-------------------"
            << std::endl;
  // TO DELETE
  set_pochi_state(false);
  fn();
  set_pochi_state(true);
  fn();
  return;
}
template <typename T>
void time_tensor(T &tensor, std::string savename) {
  {
    {
      AutoProfiler ap("compile");
      tensor.compile();
    }
    tensor.assemble();
    {
      AutoProfiler ap("compute");
      tensor.compute();
    }
  }
  write("testres_" + savename + "_" + std::to_string(new_int()) + ".tns", tensor);
}
