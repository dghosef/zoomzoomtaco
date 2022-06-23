
#include <random>
#include "taco.h"
#include "taco/util/fill.h"
#include "testutils.h"
using namespace taco;
int n = 1000;
int m = n;
int add2_test() {
  // Create a large nxm matrix with random entries
  Format csr({Dense,Sparse});
  Tensor<double> A = read("randomA.tns", csr);
  Tensor<double> B = read("randomB.tns", csr);
  Tensor<double> Result("r", {n, m});
  IndexVar i, j;
  Result(i, j) = A(i, j) + B(i, j);
  time_tensor(Result, "add2");
}

int add3_test() {
  // Create a large nxm matrix with random entries
  Format csr({Dense,Sparse});
  Tensor<double> A = read("randomA.tns", csr);
  Tensor<double> B = read("randomB.tns", csr);
  Tensor<double> C = read("randomC.tns", csr);
  Tensor<double> Result("r", {n, m});
  IndexVar i, j;
  Result(i, j) = A(i, j) + B(i, j) + C(i, j);
  time_tensor(Result, "add3");
}
