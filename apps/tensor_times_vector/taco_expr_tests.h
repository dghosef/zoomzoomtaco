
#include <random>
#include "taco.h"
#include "taco/util/fill.h"
#include "testutils.h"
using namespace taco;
int n = 1000;
int m = n;
void spmv(Tensor<double> A, Tensor<double> V) {
  // Create a large nxm matrix with random entries
  Format csr({Dense,Sparse});
  Tensor<double> Result("r", {n});
  IndexVar i, j;
  Result(i) = A(i, j) * V(i);
  time_tensor(Result, "spmv");
}
void add2(Tensor<double> A, Tensor<double> B) {
  // Create a large nxm matrix with random entries
  Format csr({Dense,Sparse});
  Tensor<double> Result("r", {n, m});
  IndexVar i, j;
  Result(i, j) = A(i, j) + B(i, j);
  time_tensor(Result, "add2");
}

void add3(Tensor<double> A, Tensor<double> B, Tensor<double> C) {
  Format csr({Dense,Sparse});
  Tensor<double> Result("result", {n, m});
  IndexVar i, j;
  Result(i, j) = A(i, j) + B(i, j) + C(i, j);
  time_tensor(Result, "add3");
}

void mul3(Tensor<double> A, Tensor<double> B, Tensor<double> C) {
  Format csr({Dense,Sparse});
  Tensor<double> Result("result", {n, m});
  IndexVar i, j;
  Result(i, j) = A(i, j) * B(i, j) * C(i, j);
  time_tensor(Result, "mul3");
}