#include <assert.h>
#include <string>
#include <fstream>

#include "taco_expr_tests.h"
#include "testutils.h"

using namespace std;


#define TIME_FN(fn) fn_with_and_without_pochi(fn, #fn)
std::string tensor_directory = "./tensors/";


Tensor<double> generate_sparse_matrix(double sparsity, std::string name) {
    std::string dimensions = std::to_string(n) + "x" + std::to_string(n);
    std:string filename = tensor_directory + name + dimensions + "sparse_" + std::to_string(sparsity)+".tns";
    Tensor<double> Res(name, {n, m});
    ifstream f;
    f.open(filename);
    if(f.good()) {
        std::cout << "reading " << name << std::endl;
        Res = read(filename, Res.getFormat(), false);
    }
    else {
        std::cout << "generating " << name << std::endl;
        taco::util::fillTensor(Res, taco::util::FillMethod::Sparse, sparsity);
        write(filename, Res);
    }
    return Res;
}
Tensor<double> generate_dense_vector(std::string name) {
    std::string dimensions = std::to_string(n);
    std:string filename = tensor_directory + name + dimensions + "dense.tns";
    Tensor<double> Res(name, {n});
    ifstream f;
    f.open(filename);
    if(f.good()) {
        std::cout << "reading " << name << std::endl;
        Res = read(filename, Res.getFormat(), false);
    }
    else {
        std::cout << "generating " << name << std::endl;
        taco::util::fillVector(Res, taco::util::FillMethod::Random, 3);
        write(filename, Res);
    }
    return Res;
}
Tensor<double> generate_dense_matrix(std::string name) {
    std::string dimensions = std::to_string(n) + "x" + std::to_string(n);
    std:string filename = tensor_directory + name + dimensions + "dense.tns";
    Tensor<double> Res(name, {n, m});
    ifstream f;
    f.open(filename);
    if(f.good()) {
        std::cout << "reading " << name << std::endl;
        Res = read(filename, Res.getFormat(), false);
    }
    else {
        std::cout << "generating " << name << std::endl;
        taco::util::fillTensor(Res, taco::util::FillMethod::Random);
        write(filename, Res);
    }
    return Res;
}
int main(int argc, char** argv) {
    assert(argc == 2);
    n = stoi(argv[1]);
    m = n;

    PochiVM::AutoThreadPochiVMContext apv;
    PochiVM::AutoThreadErrorContext arc;
    PochiVM::AutoThreadLLVMCodegenContext alc;
    set_pochi_state(false);
    Tensor<double> A = generate_sparse_matrix(0.01, "a");
    Tensor<double> B = generate_sparse_matrix(0.01, "b");
    Tensor<double> C = generate_sparse_matrix(0.01, "c");
    Tensor<double> D = generate_dense_matrix("d");
    Tensor<double> E = generate_dense_matrix("e");
    Tensor<double> V1 = generate_dense_vector("V1");
    TIME_FN([&](){add2(A, B);});
    TIME_FN([&](){add3(A, B, C);});
    TIME_FN([&](){mul3(A, D, E);});
    TIME_FN([&](){spmv(A, V1);});
    return 0;
}

