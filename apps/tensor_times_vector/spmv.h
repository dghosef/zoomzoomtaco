// On Linux and MacOS, you can compile and run this program like so:
//   g++ -std=c++11 -O3 -DNDEBUG -DTACO -I ../../include -L../../build/lib spmv.cpp -o spmv -ltaco
//   LD_LIBRARY_PATH=../../build/lib ./spmv
#include <random>
#include "taco.h"
#include "testutils.h"
using namespace taco;
int spmv_test() {
  std::default_random_engine gen(0);
  std::uniform_real_distribution<double> unif(0.0, 1.0);
  // Predeclare the storage formats that the inputs and output will be stored as.
  // To define a format, you must specify whether each dimension is dense or sparse 
  // and (optionally) the order in which dimensions should be stored. The formats 
  // declared below correspond to compressed sparse row (csr) and dense vector (dv). 
  Format csr({Dense,Sparse});
  Format  dv({Dense});

  // Load a sparse matrix from file (stored in the Matrix Market format) and 
  // store it as a compressed sparse row matrix. Matrices correspond to order-2 
  // tensors in taco. The matrix in this example can be downloaded from:
  // https://www.cise.ufl.edu/research/sparse/MM/Boeing/pwtk.tar.gz
  Tensor<double> A = read("pwtk.mtx", csr);

  // Generate a random dense vector and store it in the dense vector format. 
  // Vectors correspond to order-1 tensors in taco.
  Tensor<double> x({A.getDimension(1)}, dv);
  for (int i = 0; i < x.getDimension(0); ++i) {
    x.insert({i}, unif(gen));
  }
  x.pack();

  // Generate another random dense vetor and store it in the dense vector format..
  Tensor<double> z({A.getDimension(0)}, dv);
  for (int i = 0; i < z.getDimension(0); ++i) {
    z.insert({i}, unif(gen));
  }
  z.pack();

  // Declare and initializing the scaling factors in the SpMV computation. 
  // Scalars correspond to order-0 tensors in taco.
  Tensor<double> alpha(42.0);
  Tensor<double> beta(33.0);

  // Declare the output matrix to be a sparse matrix with the same dimensions as 
  // input matrix B, to be also stored as a doubly compressed sparse row matrix.
  Tensor<double> y({A.getDimension(0)}, dv);
  // Define the SpMV computation using index notation.
  IndexVar i, j;
  y(i) = alpha() * (A(i,j) * x(j)) + beta() * z(i);
  // At this point, we have defined how entries in the output vector should be 
  // computed from entries in the input matrice and vectorsbut have not actually 
  // performed the computation yet. To do so, we must first tell taco to generate 
  // code that can be executed to compute the SpMV operation.
  time_tensor(y, "spmv");
}
int spmv_2_test() {
  set_pochi_state(false);
  std::default_random_engine gen(0);
  std::uniform_real_distribution<double> unif(0.0, 1.0);
  // Predeclare the storage formats that the inputs and output will be stored as.
  // To define a format, you must specify whether each dimension is dense or sparse 
  // and (optionally) the order in which dimensions should be stored. The formats 
  // declared below correspond to compressed sparse row (csr) and dense vector (dv). 
  Format csr({Dense,Sparse});
  Format  dv({Dense});

  // Load a sparse matrix from file (stored in the Matrix Market format) and 
  // store it as a compressed sparse row matrix. Matrices correspond to order-2 
  // tensors in taco. The matrix in this example can be downloaded from:
  // https://www.cise.ufl.edu/research/sparse/MM/Boeing/pwtk.tar.gz
  Tensor<double> A = read("pwtk.mtx", csr);

  // Generate a random dense vector and store it in the dense vector format. 
  // Vectors correspond to order-1 tensors in taco.
  Tensor<double> x({A.getDimension(1)}, dv);
  for (int i = 0; i < x.getDimension(0); ++i) {
    x.insert({i}, unif(gen));
  }
  x.pack();

  // Generate another random dense vetor and store it in the dense vector format..
  Tensor<double> z({A.getDimension(0)}, dv);
  for (int i = 0; i < z.getDimension(0); ++i) {
    z.insert({i}, unif(gen));
  }
  z.pack();

  // Declare and initializing the scaling factors in the SpMV computation. 
  // Scalars correspond to order-0 tensors in taco.
  Tensor<double> alpha(42.0);
  Tensor<double> beta(33.0);

  // Declare the output matrix to be a sparse matrix with the same dimensions as 
  // input matrix B, to be also stored as a doubly compressed sparse row matrix.
  Tensor<double> y({A.getDimension(0)}, dv);
  // Define the SpMV computation using index notation.
  IndexVar i, j;
  y(i) = (A(i,j) * x(j));
  // At this point, we have defined how entries in the output vector should be 
  // computed from entries in the input matrice and vectorsbut have not actually 
  // performed the computation yet. To do so, we must first tell taco to generate 
  // code that can be executed to compute the SpMV operation.
  y.compile();
}