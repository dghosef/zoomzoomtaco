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
  // hack because for some reason we segfault in one of the destructors
  pid_t pid1 = fork();
  if (pid1 == 0) {
    std::cout << "==========================================================" << std::endl;
    std::cout << "with pochi" << std::endl;
    set_pochi_state(true);
    fn();
  } else {
    waitpid(pid1, nullptr, 0);
    std::cout << "==========================================================" << std::endl;
    std::cout << "without pochi" << std::endl;
    pid_t pid2 = fork();
    if (pid2 == 0) {
      set_pochi_state(false);
      fn();
    } else {
      waitpid(pid2, nullptr, 0);
    }
  }
  std::cout << "==========================================================" << std::endl;
}
template <typename T>
[[noreturn]] void time_tensor(T &tensor, std::string savename) {
  {
    {
      AutoProfiler ap("compile");
      tensor.compile();
    }
    {
      AutoProfiler ap("assemble");
      tensor.assemble();
    }
    {
      AutoProfiler ap("compute");
      tensor.compute();
    }
  }
  write("testres_" + savename + "_" + std::to_string(getpid()) + ".tns", tensor);
  
  exit(0);
}
