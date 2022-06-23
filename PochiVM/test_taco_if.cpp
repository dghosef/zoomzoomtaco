#include "gtest/gtest.h"

#include "codegen_context.hpp"
#include "pochivm.h"
#include "test_util_helper.h"

using namespace PochiVM;
TEST(Sanity, TestIncrIfEqual) {
    AutoTimer tm;
  AutoThreadPochiVMContext apv;
  AutoThreadErrorContext arc;
  AutoThreadLLVMCodegenContext alc;
  using FnPrototype1 = int (*)(int, int, int);

  thread_pochiVMContext->m_curModule = new AstModule("test");
  {
    auto [fn, a, b, c] = NewFunction<FnPrototype1>("inc_value");
    fn.SetBody(IncrIfEq(a, b, c), Return(a));
  }
  ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
  ReleaseAssert(!thread_errorContext->HasError());

  thread_pochiVMContext->m_curModule->PrepareForFastInterp();
  {
    FastInterpFunction<FnPrototype1> interpFn =
        thread_pochiVMContext->m_curModule
            ->GetFastInterpGeneratedFunction<FnPrototype1>("inc_value");

    int result = interpFn(4, 4, 4);
    ReleaseAssert(result == 5);
    result = interpFn(4, 5, 4);
    ReleaseAssert(result == 4);
  }
}

TEST(Sanity, GOTO) {
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;
    using FnPrototype1 = int (*)();
  thread_pochiVMContext->m_curModule = new AstModule("test-goto");
  {
    auto [fn] = NewFunction<FnPrototype1>("test_goto");
    // auto var = fn.NewVariable<int>();
    // should loop forever
    fn.SetBody(
        While(PochiVM::Literal<bool>(true)).Do(
            Goto("lbl")
        ),
        Label("lbl"),
        Return(Literal<int>(4))
    );
  }
  ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
  ReleaseAssert(!thread_errorContext->HasError());

  thread_pochiVMContext->m_curModule->PrepareForFastInterp();
    FastInterpFunction<FnPrototype1> interpFn =
        thread_pochiVMContext->m_curModule
            ->GetFastInterpGeneratedFunction<FnPrototype1>("test_goto");
    interpFn();
}
