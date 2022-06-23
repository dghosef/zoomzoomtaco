#include "spmv.h"
#include "add2.h"
#include "testutils.h"


#define TIME_FN(fn) fn_with_and_without_pochi(fn, #fn)
int main() {
  // TIME_FN(tensor_times_vector);

  PochiVM::AutoThreadPochiVMContext apv;
  PochiVM::AutoThreadErrorContext arc;
  PochiVM::AutoThreadLLVMCodegenContext alc;
  // std::cout << "Making large random matrices" << std::endl;
  // set_pochi_state(false);
  // Tensor<double> A("a", {n, m});
  // Tensor<double> B("b", {n, m});
  // Tensor<double> C("c", {n, m});
  // taco::util::fillTensor(A, taco::util::FillMethod::Random);
  // taco::util::fillTensor(B, taco::util::FillMethod::Random);
  // taco::util::fillTensor(C, taco::util::FillMethod::Random);
  // write("randomA.tns", A);
  // write("randomB.tns", B);
  // write("randomC.tns", C);
  // std::cout << "Done making large random matrices" << std::endl;
  // TIME_FN(spmv_test);
  // TIME_FN(add2_test);
  // TIME_FN(add3_test);
  spmv_2_test();
  return 0;
}

