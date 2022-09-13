#include "gtest/gtest.h"

#include "pochivm.h"
#include "test_util_helper.h"
#include "codegen_context.hpp"

using namespace PochiVM;

TEST(SanityCallCppFn, Sanity_1)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(TestClassA*, int);
    {
        auto [fn, c, v] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                c->SetY(v + Literal<int>(1)),
                Return((*c).GetY() + Literal<int>(2))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");
        TestClassA a;
        int ret = interpFn(&a, 123);
        ReleaseAssert(a.m_y == 123 + 1);
        ReleaseAssert(ret == 123 + 3);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        TestClassA a;
        int ret = interpFn(&a, 123);
        ReleaseAssert(a.m_y == 123 + 1);
        ReleaseAssert(ret == 123 + 3);
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        if (x_isDebugBuild)
        {
            jit.SetAllowResolveSymbolInHostProcess(true);
        }
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        TestClassA a;
        int ret = jitFn(&a, 123);
        ReleaseAssert(a.m_y == 123 + 1);
        ReleaseAssert(ret == 123 + 3);
    }
}

TEST(SanityCallCppFn, Sanity_2)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int64_t(*)(TestClassA*, int);
    {
        auto [fn, c, v] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                c->PushVec(v),
                Return(c->GetVectorSum())
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");
        TestClassA a;
        int expectedSum = 0;
        std::vector<int> expectedVec;
        for (int i = 0; i < 100; i++)
        {
            int k = rand() % 1000;
            int64_t ret = interpFn(&a, k);
            expectedVec.push_back(k);
            expectedSum += k;
            ReleaseAssert(ret == expectedSum);
            ReleaseAssert(a.m_vec == expectedVec);
        }
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        TestClassA a;
        int expectedSum = 0;
        std::vector<int> expectedVec;
        for (int i = 0; i < 100; i++)
        {
            int k = rand() % 1000;
            int64_t ret = interpFn(&a, k);
            expectedVec.push_back(k);
            expectedSum += k;
            ReleaseAssert(ret == expectedSum);
            ReleaseAssert(a.m_vec == expectedVec);
        }
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "before_opt");
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        TestClassA a;
        int expectedSum = 0;
        std::vector<int> expectedVec;
        for (int i = 0; i < 100; i++)
        {
            int k = rand() % 1000;
            int64_t ret = jitFn(&a, k);
            expectedVec.push_back(k);
            expectedSum += k;
            ReleaseAssert(ret == expectedSum);
            ReleaseAssert(a.m_vec == expectedVec);
        }
    }
}

TEST(SanityCallCppFn, TypeMismatchErrorCase)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = double(*)(TestClassA*);
    {
        auto [fn, c] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                Return(c->GetVectorSum())
        );
    }

    ReleaseAssert(!thread_pochiVMContext->m_curModule->Validate());
    ReleaseAssert(thread_errorContext->HasError());
    AssertIsExpectedOutput(thread_errorContext->m_errorMsg);
}

TEST(SanityCallCppFn, Sanity_3)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(std::string*);
    {
        auto [fn, c] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                Return(CallFreeFn::FreeFnRecursive2(*c))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");
        std::string val = "10";
        int result = interpFn(&val);
        ReleaseAssert(result == 89);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        std::string val = "10";
        int result = interpFn(&val);
        ReleaseAssert(result == 89);
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "before_opt");
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        std::string val = "10";
        int result = jitFn(&val);
        ReleaseAssert(result == 89);
    }
}

TEST(SanityCallCppFn, UnusedCppTypeCornerCase)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void*(*)(std::string*);
    {
        auto [fn, c] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                Return(ReinterpretCast<void*>(c))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");
        std::string val = "10";
        void* result = interpFn(&val);
        ReleaseAssert(result == reinterpret_cast<void*>(&val));
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        std::string val = "10";
        void* result = interpFn(&val);
        ReleaseAssert(result == reinterpret_cast<void*>(&val));
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "before_opt");
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        std::string val = "10";
        void* result = jitFn(&val);
        ReleaseAssert(result == reinterpret_cast<void*>(&val));
    }
}

TEST(SanityCallCppFn, BooleanTypeCornerCase_1)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = bool(*)(int, bool*, bool**);
    {
        auto [fn, a, b, c] = NewFunction<FnPrototype>("testfn");
        auto d = fn.NewVariable<bool>();

        fn.SetBody(
                Declare(d, a == Literal<int>(233)),
                Return(CallFreeFn::TestCornerCases::BoolParamTest1(d, b, c))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        bool x[2];
        bool* px = x;
        bool** ppx = &px;
        bool b[3];
        uint8_t* _b = reinterpret_cast<uint8_t*>(b);

        memset(b, 233, 3);
        x[0] = true; x[1] = false;
        ReleaseAssert(interpFn(233, b, ppx) == false);
        ReleaseAssert(_b[0] == 1);
        ReleaseAssert(_b[1] == 1);
        ReleaseAssert(_b[2] == 1);

        memset(b, 233, 3);
        x[0] = false; x[1] = true;
        ReleaseAssert(interpFn(100, b, ppx) == true);
        ReleaseAssert(_b[0] == 1);
        ReleaseAssert(_b[1] == 0);
        ReleaseAssert(_b[2] == 0);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        bool x[2];
        bool* px = x;
        bool** ppx = &px;
        bool b[3];
        uint8_t* _b = reinterpret_cast<uint8_t*>(b);

        memset(b, 233, 3);
        x[0] = true; x[1] = false;
        ReleaseAssert(interpFn(233, b, ppx) == false);
        ReleaseAssert(_b[0] == 1);
        ReleaseAssert(_b[1] == 1);
        ReleaseAssert(_b[2] == 1);

        memset(b, 233, 3);
        x[0] = false; x[1] = true;
        ReleaseAssert(interpFn(100, b, ppx) == true);
        ReleaseAssert(_b[0] == 1);
        ReleaseAssert(_b[1] == 0);
        ReleaseAssert(_b[2] == 0);
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        bool x[2];
        bool* px = x;
        bool** ppx = &px;
        bool b[3];
        uint8_t* _b = reinterpret_cast<uint8_t*>(b);

        memset(b, 233, 3);
        x[0] = true; x[1] = false;
        ReleaseAssert(jitFn(233, b, ppx) == false);
        ReleaseAssert(_b[0] == 1);
        ReleaseAssert(_b[1] == 1);
        ReleaseAssert(_b[2] == 1);

        memset(b, 233, 3);
        x[0] = false; x[1] = true;
        ReleaseAssert(jitFn(100, b, ppx) == true);
        ReleaseAssert(_b[0] == 1);
        ReleaseAssert(_b[1] == 0);
        ReleaseAssert(_b[2] == 0);
    }
}

TEST(SanityCallCppFn, BooleanTypeCornerCase_2)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = bool(*)(int, bool**, bool**);
    {
        auto [fn, a, b, c] = NewFunction<FnPrototype>("testfn");
        auto d = fn.NewVariable<bool>();

        fn.SetBody(
                Declare(d, a == Literal<int>(233)),
                CallFreeFn::TestCornerCases::BoolParamTest2(d, *b, c),
                Return(d)
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        {
            bool x[2];
            bool* px = x;
            bool** ppx = &px;
            bool b[4];
            bool* pb = b;
            bool** ppb = &pb;
            uint8_t* _b = reinterpret_cast<uint8_t*>(b);

            memset(x, 233, 1); x[1] = false;
            memset(b, 233, 4);
            ReleaseAssert(interpFn(233, ppx, ppb) == false);
            ReleaseAssert(*reinterpret_cast<uint8_t*>(x) == 1);
            ReleaseAssert(_b[2] == 1);
            ReleaseAssert(_b[3] == 0);
            ReleaseAssert(*ppx = pb);
        }
        {
            bool x[2];
            bool* px = x;
            bool** ppx = &px;
            bool b[4];
            bool* pb = b;
            bool** ppb = &pb;
            uint8_t* _b = reinterpret_cast<uint8_t*>(b);
            memset(x, 233, 1); x[1] = true;
            memset(b, 233, 4); b[1] = false;
            ReleaseAssert(interpFn(100, ppx, ppb) == true);
            ReleaseAssert(*reinterpret_cast<uint8_t*>(x) == 0);
            ReleaseAssert(_b[2] == 1);
            ReleaseAssert(_b[3] == 0);
            ReleaseAssert(*ppx = pb);
        }
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        {
            bool x[2];
            bool* px = x;
            bool** ppx = &px;
            bool b[4];
            bool* pb = b;
            bool** ppb = &pb;
            uint8_t* _b = reinterpret_cast<uint8_t*>(b);

            memset(x, 233, 1); x[1] = false;
            memset(b, 233, 4);
            ReleaseAssert(interpFn(233, ppx, ppb) == false);
            ReleaseAssert(*reinterpret_cast<uint8_t*>(x) == 1);
            ReleaseAssert(_b[2] == 1);
            ReleaseAssert(_b[3] == 0);
            ReleaseAssert(*ppx = pb);
        }
        {
            bool x[2];
            bool* px = x;
            bool** ppx = &px;
            bool b[4];
            bool* pb = b;
            bool** ppb = &pb;
            uint8_t* _b = reinterpret_cast<uint8_t*>(b);
            memset(x, 233, 1); x[1] = true;
            memset(b, 233, 4); b[1] = false;
            ReleaseAssert(interpFn(100, ppx, ppb) == true);
            ReleaseAssert(*reinterpret_cast<uint8_t*>(x) == 0);
            ReleaseAssert(_b[2] == 1);
            ReleaseAssert(_b[3] == 0);
            ReleaseAssert(*ppx = pb);
        }
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");
        {
            bool x[2];
            bool* px = x;
            bool** ppx = &px;
            bool b[4];
            bool* pb = b;
            bool** ppb = &pb;
            uint8_t* _b = reinterpret_cast<uint8_t*>(b);

            memset(x, 233, 1); x[1] = false;
            memset(b, 233, 4);
            ReleaseAssert(jitFn(233, ppx, ppb) == false);
            ReleaseAssert(*reinterpret_cast<uint8_t*>(x) == 1);
            ReleaseAssert(_b[2] == 1);
            ReleaseAssert(_b[3] == 0);
            ReleaseAssert(*ppx = pb);
        }
        {
            bool x[2];
            bool* px = x;
            bool** ppx = &px;
            bool b[4];
            bool* pb = b;
            bool** ppb = &pb;
            uint8_t* _b = reinterpret_cast<uint8_t*>(b);
            memset(x, 233, 1); x[1] = true;
            memset(b, 233, 4); b[1] = false;
            ReleaseAssert(jitFn(100, ppx, ppb) == true);
            ReleaseAssert(*reinterpret_cast<uint8_t*>(x) == 0);
            ReleaseAssert(_b[2] == 1);
            ReleaseAssert(_b[3] == 0);
            ReleaseAssert(*ppx = pb);
        }
    }
}

TEST(SanityCallCppFn, VoidStarCornerCase_1)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void*(*)(void*, void**);
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("testfn");
        fn.SetBody(
                Return(CallFreeFn::TestCornerCases::VoidStarParamTest1(a, b))
       );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        void* a = reinterpret_cast<void*>(233);
        void* c;
        void** b = &c;
        ReleaseAssert(interpFn(a, b) == reinterpret_cast<void*>(233 + 8));
        ReleaseAssert(c == reinterpret_cast<void*>(233));
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        void* a = reinterpret_cast<void*>(233);
        void* c;
        void** b = &c;
        ReleaseAssert(interpFn(a, b) == reinterpret_cast<void*>(233 + 8));
        ReleaseAssert(c == reinterpret_cast<void*>(233));
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        void* a = reinterpret_cast<void*>(233);
        void* c;
        void** b = &c;
        ReleaseAssert(jitFn(a, b) == reinterpret_cast<void*>(233 + 8));
        ReleaseAssert(c == reinterpret_cast<void*>(233));
    }
}

TEST(SanityCallCppFn, VoidStarCornerCase_2)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void*(*)(void*, void**);
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("testfn");
        fn.SetBody(
                CallFreeFn::TestCornerCases::VoidStarParamTest2(a, b),
                Return(a)
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        void* a = reinterpret_cast<void*>(233);
        void* b[2];
        b[0] = nullptr; b[1] = reinterpret_cast<void*>(345);
        ReleaseAssert(interpFn(a, b) == reinterpret_cast<void*>(345));
        ReleaseAssert(b[0] == reinterpret_cast<void*>(233));
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        void* a = reinterpret_cast<void*>(233);
        void* b[2];
        b[0] = nullptr; b[1] = reinterpret_cast<void*>(345);
        ReleaseAssert(interpFn(a, b) == reinterpret_cast<void*>(345));
        ReleaseAssert(b[0] == reinterpret_cast<void*>(233));
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        void* a = reinterpret_cast<void*>(233);
        void* b[2];
        b[0] = nullptr; b[1] = reinterpret_cast<void*>(345);
        ReleaseAssert(jitFn(a, b) == reinterpret_cast<void*>(345));
        ReleaseAssert(b[0] == reinterpret_cast<void*>(233));
    }
}

TEST(SanityCallCppFn, ReturnsNonPrimitiveType)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = bool(*)(int);
    {
        auto [fn, param] = NewFunction<FnPrototype>("testfn");
        auto v = fn.NewVariable<TestNonTrivialConstructor>();
        fn.SetBody(
                Declare(v, Variable<TestNonTrivialConstructor>::Create(param)),
                Return(v.GetValue() == Literal<int>(233))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        ReleaseAssert(interpFn(233) == true);
        ReleaseAssert(interpFn(234) == false);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        ReleaseAssert(interpFn(233) == true);
        ReleaseAssert(interpFn(234) == false);
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        // in non-debug build, the whole thing should have been inlined and optimized out
        //
        jit.SetAllowResolveSymbolInHostProcess(x_isDebugBuild);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        ReleaseAssert(jitFn(233) == true);
        ReleaseAssert(jitFn(234) == false);
    }
}

TEST(SanityCallCppFn, NonTrivialCopyConstructor)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(TestNonTrivialCopyConstructor*);
    {
        auto [fn, param] = NewFunction<FnPrototype>("testfn");
        fn.SetBody(
                Return(Variable<TestNonTrivialCopyConstructor>::Fn(*param))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        TestNonTrivialCopyConstructor::counter = 0;
        TestNonTrivialCopyConstructor x(5);
        ReleaseAssert(interpFn(&x) == 5);
        ReleaseAssert(TestNonTrivialCopyConstructor::counter == 1);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        TestNonTrivialCopyConstructor::counter = 0;
        TestNonTrivialCopyConstructor x(5);
        ReleaseAssert(interpFn(&x) == 5);
        ReleaseAssert(TestNonTrivialCopyConstructor::counter == 1);
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        TestNonTrivialCopyConstructor::counter = 0;
        TestNonTrivialCopyConstructor x(5);
        ReleaseAssert(jitFn(&x) == 5);
        ReleaseAssert(TestNonTrivialCopyConstructor::counter == 1);
    }
}

TEST(SanityCallCppFn, Constructor_1)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(std::vector<int>*);
    {
        auto [fn, param] = NewFunction<FnPrototype>("testfn");
        auto v = fn.NewVariable<std::vector<int>>();
        fn.SetBody(
                Declare(v),
                CallFreeFn::CopyVectorInt(param, v.Addr())
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        std::vector<int> a;
        a.push_back(233);
        interpFn(&a);
        ReleaseAssert(a.size() == 0);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        std::vector<int> a;
        a.push_back(233);
        interpFn(&a);
        ReleaseAssert(a.size() == 0);
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        std::vector<int> a;
        a.push_back(233);
        jitFn(&a);
        ReleaseAssert(a.size() == 0);
    }
}

TEST(SanityCallCppFn, Constructor_2)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(std::vector<int>*, size_t);
    {
        auto [fn, param, num] = NewFunction<FnPrototype>("testfn");
        auto v = fn.NewVariable<std::vector<int>>();
        fn.SetBody(
                Declare(v, Constructor<std::vector<int>>(num)),
                CallFreeFn::CopyVectorInt(param, v.Addr())
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        std::vector<int> a;
        a.push_back(233);
        interpFn(&a, 100);
        ReleaseAssert(a.size() == 100);
        for (size_t i = 0; i < 100; i++) { ReleaseAssert(a[i] == 0); }
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        std::vector<int> a;
        a.push_back(233);
        interpFn(&a, 100);
        ReleaseAssert(a.size() == 100);
        for (size_t i = 0; i < 100; i++) { ReleaseAssert(a[i] == 0); }
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        std::vector<int> a;
        a.push_back(233);
        jitFn(&a, 100);
        ReleaseAssert(a.size() == 100);
        for (size_t i = 0; i < 100; i++) { ReleaseAssert(a[i] == 0); }
    }
}

TEST(SanityCallCppFn, Constructor_3)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(std::vector<int>*, size_t, int);
    {
        auto [fn, param, num, val] = NewFunction<FnPrototype>("testfn");
        auto v = fn.NewVariable<std::vector<int>>();
        fn.SetBody(
                Declare(v, Constructor<std::vector<int>>(num, val)),
                CallFreeFn::CopyVectorInt(param, v.Addr())
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        std::vector<int> a;
        a.push_back(233);
        interpFn(&a, 100, 34567);
        ReleaseAssert(a.size() == 100);
        for (size_t i = 0; i < 100; i++) { ReleaseAssert(a[i] == 34567); }
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        std::vector<int> a;
        a.push_back(233);
        interpFn(&a, 100, 34567);
        ReleaseAssert(a.size() == 100);
        for (size_t i = 0; i < 100; i++) { ReleaseAssert(a[i] == 34567); }
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        std::vector<int> a;
        a.push_back(233);
        jitFn(&a, 100, 45678);
        ReleaseAssert(a.size() == 100);
        for (size_t i = 0; i < 100; i++) { ReleaseAssert(a[i] == 45678); }
    }
}

TEST(SanityCallCppFn, Constructor_4)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(std::vector<int>*, std::vector<int>*);
    {
        auto [fn, param, param2] = NewFunction<FnPrototype>("testfn");
        auto v = fn.NewVariable<std::vector<int>>();
        fn.SetBody(
                Declare(v, Constructor<std::vector<int>>(*param2)),
                CallFreeFn::CopyVectorInt(param, v.Addr())
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        std::vector<int> a, b;
        a.push_back(233);
        b.push_back(456); b.push_back(567);
        interpFn(&a, &b);
        ReleaseAssert(a.size() == 2);
        ReleaseAssert(a[0] == 456 && a[1] == 567);
        ReleaseAssert(b.size() == 2);
        ReleaseAssert(b[0] == 456 && b[1] == 567);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        std::vector<int> a, b;
        a.push_back(233);
        b.push_back(456); b.push_back(567);
        interpFn(&a, &b);
        ReleaseAssert(a.size() == 2);
        ReleaseAssert(a[0] == 456 && a[1] == 567);
        ReleaseAssert(b.size() == 2);
        ReleaseAssert(b[0] == 456 && b[1] == 567);
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        std::vector<int> a, b;
        a.push_back(233);
        b.push_back(456); b.push_back(567);
        jitFn(&a, &b);
        ReleaseAssert(a.size() == 2);
        ReleaseAssert(a[0] == 456 && a[1] == 567);
        ReleaseAssert(b.size() == 2);
        ReleaseAssert(b[0] == 456 && b[1] == 567);
    }
}

TEST(SanityCallCppFn, Constructor_5)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)();
    {
        auto [fn] = NewFunction<FnPrototype>("testfn");
        auto v = fn.NewVariable<TestConstructor1>();
        fn.SetBody(
                Declare(v),
                Return(v.GetValue())
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        ReleaseAssert(interpFn() == 233);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        ReleaseAssert(interpFn() == 233);
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        ReleaseAssert(jitFn() == 233);
    }
}

TEST(SanityCallCppFn, ManuallyCallDestructor)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(TestDestructor1*);
    {
        auto [fn, v] = NewFunction<FnPrototype>("testfn");
        fn.SetBody(
                CallDestructor(v)
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        int out = 0;
        TestDestructor1* obj = reinterpret_cast<TestDestructor1*>(alloca(sizeof(TestDestructor1)));
        new (obj) TestDestructor1(233, &out);
        ReleaseAssert(out == 0);
        interpFn(obj);
        ReleaseAssert(out == 233);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        int out = 0;
        TestDestructor1* obj = reinterpret_cast<TestDestructor1*>(alloca(sizeof(TestDestructor1)));
        new (obj) TestDestructor1(233, &out);
        ReleaseAssert(out == 0);
        interpFn(obj);
        ReleaseAssert(out == 233);
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(x_isDebugBuild);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        int out = 0;
        TestDestructor1* obj = reinterpret_cast<TestDestructor1*>(alloca(sizeof(TestDestructor1)));
        new (obj) TestDestructor1(233, &out);
        ReleaseAssert(out == 0);
        jitFn(obj);
        ReleaseAssert(out == 233);
    }
}

TEST(SanityCallCppFn, DestructorSanity_1)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(int, int*);
    {
        auto [fn, v, addr] = NewFunction<FnPrototype>("testfn");
        auto x = fn.NewVariable<TestDestructor1>();
        fn.SetBody(
                Declare(x, Constructor<TestDestructor1>(v, addr))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        int out = 0;
        interpFn(233, &out);
        ReleaseAssert(out == 233);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        int out = 0;
        interpFn(233, &out);
        ReleaseAssert(out == 233);
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(x_isDebugBuild);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        int out = 0;
        jitFn(233, &out);
        ReleaseAssert(out == 233);
    }
}

TEST(SanityCallCppFn, DestructorSanity_2)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(CtorDtorOrderRecorder*);
    {
        auto [fn, r] = NewFunction<FnPrototype>("testfn");
        auto x = fn.NewVariable<TestDestructor2>();
        fn.SetBody(
                Declare(x, Constructor<TestDestructor2>(r, Literal<int>(123)))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        CtorDtorOrderRecorder r;
        interpFn(&r);
        ReleaseAssert((r.order == std::vector<int>{123, -123}));
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        CtorDtorOrderRecorder r;
        interpFn(&r);
        ReleaseAssert((r.order == std::vector<int>{123, -123}));
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        CtorDtorOrderRecorder r;
        jitFn(&r);
        ReleaseAssert((r.order == std::vector<int>{123, -123}));
    }
}

TEST(SanityCallCppFn, DestructorCalledInReverseOrder_1)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(CtorDtorOrderRecorder*);
    {
        auto [fn, r] = NewFunction<FnPrototype>("testfn");
        auto v1 = fn.NewVariable<TestDestructor2>();
        auto v2 = fn.NewVariable<TestDestructor2>();
        auto v3 = fn.NewVariable<TestDestructor2>();
        auto v4 = fn.NewVariable<TestDestructor2>();
        fn.SetBody(
                r->Push(Literal<int>(1000)),
                Declare(v1, Constructor<TestDestructor2>(r, Literal<int>(1))),
                r->Push(Literal<int>(1001)),
                Declare(v2, Constructor<TestDestructor2>(r, Literal<int>(2))),
                r->Push(Literal<int>(1002)),
                Declare(v3, Constructor<TestDestructor2>(r, Literal<int>(3))),
                r->Push(Literal<int>(1003)),
                Declare(v4, Constructor<TestDestructor2>(r, Literal<int>(4))),
                r->Push(Literal<int>(1004))
        );
    }

    std::vector<int> expectedAns {
            1000, 1, 1001, 2, 1002, 3, 1003, 4, 1004, -4, -3, -2, -1
    };

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        CtorDtorOrderRecorder r;
        interpFn(&r);
        ReleaseAssert(r.order == expectedAns);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        CtorDtorOrderRecorder r;
        interpFn(&r);
        ReleaseAssert(r.order == expectedAns);
    }

    thread_pochiVMContext->m_curModule->EmitIR();
    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        CtorDtorOrderRecorder r;
        jitFn(&r);
        ReleaseAssert(r.order == expectedAns);
    }
}

TEST(SanityCallCppFn, DestructorCalledInReverseOrder_2)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(CtorDtorOrderRecorder*);
    {
        auto [fn, r] = NewFunction<FnPrototype>("testfn");
        auto v1 = fn.NewVariable<TestDestructor2>();
        auto v2 = fn.NewVariable<TestDestructor2>();
        auto v3 = fn.NewVariable<TestDestructor2>();
        auto v4 = fn.NewVariable<TestDestructor2>();
        auto v5 = fn.NewVariable<TestDestructor2>();
        auto v6 = fn.NewVariable<TestDestructor2>();
        auto v7 = fn.NewVariable<TestDestructor2>();
        fn.SetBody(
                r->Push(Literal<int>(1000)),
                Declare(v1, Constructor<TestDestructor2>(r, Literal<int>(1))),
                r->Push(Literal<int>(1001)),
                Scope(
                    r->Push(Literal<int>(1002)),
                    Declare(v2, Constructor<TestDestructor2>(r, Literal<int>(2))),
                    r->Push(Literal<int>(1003)),
                    Declare(v3, Constructor<TestDestructor2>(r, Literal<int>(3))),
                    r->Push(Literal<int>(1004))
                ),
                (*r).Push(Literal<int>(1005)),
                Declare(v4, Constructor<TestDestructor2>(r, Literal<int>(4))),
                r->Push(Literal<int>(1006)),
                Scope(
                    r->Push(Literal<int>(1007)),
                    Declare(v5, Constructor<TestDestructor2>(r, Literal<int>(5))),
                    r->Push(Literal<int>(1008)),
                    Declare(v6, Constructor<TestDestructor2>(r, Literal<int>(6))),
                    r->Push(Literal<int>(1009))
                ),
                r->Push(Literal<int>(1010)),
                Declare(v7, Constructor<TestDestructor2>(r, Literal<int>(7))),
                r->Push(Literal<int>(1011))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    std::vector<int> expectedAns {
            1000, 1, 1001, 1002, 2, 1003, 3, 1004, -3, -2, 1005, 4, 1006,
            1007, 5, 1008, 6, 1009, -6, -5, 1010, 7, 1011, -7, -4, -1
    };

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        CtorDtorOrderRecorder r;
        interpFn(&r);
        ReleaseAssert(r.order == expectedAns);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        CtorDtorOrderRecorder r;
        interpFn(&r);
        ReleaseAssert(r.order == expectedAns);
    }

    thread_pochiVMContext->m_curModule->EmitIR();
    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        CtorDtorOrderRecorder r;
        jitFn(&r);
        ReleaseAssert(r.order == expectedAns);
    }
}

TEST(SanityCallCppFn, BlockDoesNotConstituteVariableScope)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(CtorDtorOrderRecorder*);
    {
        auto [fn, r] = NewFunction<FnPrototype>("testfn");
        auto v1 = fn.NewVariable<TestDestructor2>();
        auto v2 = fn.NewVariable<TestDestructor2>();
        auto v3 = fn.NewVariable<TestDestructor2>();
        auto v4 = fn.NewVariable<TestDestructor2>();
        auto v5 = fn.NewVariable<TestDestructor2>();
        auto v6 = fn.NewVariable<TestDestructor2>();
        auto v7 = fn.NewVariable<TestDestructor2>();
        fn.SetBody(
                r->Push(Literal<int>(1000)),
                Declare(v1, Constructor<TestDestructor2>(r, Literal<int>(1))),
                r->Push(Literal<int>(1001)),
                Block(
                    r->Push(Literal<int>(1002)),
                    Declare(v2, Constructor<TestDestructor2>(r, Literal<int>(2))),
                    r->Push(Literal<int>(1003)),
                    Declare(v3, Constructor<TestDestructor2>(r, Literal<int>(3))),
                    r->Push(Literal<int>(1004))
                ),
                r->Push(Literal<int>(1005)),
                Declare(v4, Constructor<TestDestructor2>(r, Literal<int>(4))),
                r->Push(Literal<int>(1006)),
                Block(
                    r->Push(Literal<int>(1007)),
                    Declare(v5, Constructor<TestDestructor2>(r, Literal<int>(5))),
                    r->Push(Literal<int>(1008)),
                    Declare(v6, Constructor<TestDestructor2>(r, Literal<int>(6))),
                    r->Push(Literal<int>(1009))
                ),
                r->Push(Literal<int>(1010)),
                Declare(v7, Constructor<TestDestructor2>(r, Literal<int>(7))),
                r->Push(Literal<int>(1011))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    std::vector<int> expectedAns {
            1000, 1, 1001, 1002, 2, 1003, 3, 1004, 1005, 4, 1006, 1007, 5, 1008, 6, 1009,
            1010, 7, 1011, -7, -6, -5, -4, -3, -2, -1
    };

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        CtorDtorOrderRecorder r;
        interpFn(&r);
        ReleaseAssert(r.order == expectedAns);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        CtorDtorOrderRecorder r;
        interpFn(&r);
        ReleaseAssert(r.order == expectedAns);
    }

    thread_pochiVMContext->m_curModule->EmitIR();
    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        CtorDtorOrderRecorder r;
        jitFn(&r);
        ReleaseAssert(r.order == expectedAns);
    }
}

TEST(SanityCallCppFn, DestructorCalledUponReturnStmt)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(CtorDtorOrderRecorder*);
    {
        auto [fn, r] = NewFunction<FnPrototype>("testfn");
        auto v1 = fn.NewVariable<TestDestructor2>();
        auto v2 = fn.NewVariable<TestDestructor2>();
        auto v3 = fn.NewVariable<TestDestructor2>();
        auto v4 = fn.NewVariable<TestDestructor2>();
        auto v5 = fn.NewVariable<TestDestructor2>();
        auto v6 = fn.NewVariable<TestDestructor2>();
        auto v7 = fn.NewVariable<TestDestructor2>();
        auto v8 = fn.NewVariable<TestDestructor2>();
        auto v9 = fn.NewVariable<TestDestructor2>();
        fn.SetBody(
                r->Push(Literal<int>(1000)),
                Declare(v1, Constructor<TestDestructor2>(r, Literal<int>(1))),
                r->Push(Literal<int>(1001)),
                Declare(v2, Constructor<TestDestructor2>(r, Literal<int>(2))),
                r->Push(Literal<int>(1002)),
                Scope(
                    r->Push(Literal<int>(1003)),
                    Declare(v3, Constructor<TestDestructor2>(r, Literal<int>(3))),
                    r->Push(Literal<int>(1004)),
                    Declare(v4, Constructor<TestDestructor2>(r, Literal<int>(4))),
                    r->Push(Literal<int>(1005)),
                    Scope(
                        r->Push(Literal<int>(1006)),
                        Declare(v5, Constructor<TestDestructor2>(r, Literal<int>(5))),
                        r->Push(Literal<int>(1007)),
                        Declare(v6, Constructor<TestDestructor2>(r, Literal<int>(6))),
                        r->Push(Literal<int>(1008))
                    ),
                    r->Push(Literal<int>(1009)),
                    Declare(v7, Constructor<TestDestructor2>(r, Literal<int>(7))),
                    r->Push(Literal<int>(1010)),
                    Scope(
                        r->Push(Literal<int>(1011)),
                        Declare(v8, Constructor<TestDestructor2>(r, Literal<int>(8))),
                        r->Push(Literal<int>(1012)),
                        Declare(v9, Constructor<TestDestructor2>(r, Literal<int>(9))),
                        r->Push(Literal<int>(1013)),
                        Return()
                    )
                )
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    std::vector<int> expectedAns {
            1000, 1, 1001, 2, 1002, 1003, 3, 1004, 4, 1005, 1006, 5, 1007, 6, 1008,
            -6, -5, 1009, 7, 1010, 1011, 8, 1012, 9, 1013, -9, -8, -7, -4, -3, -2, -1
    };

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        CtorDtorOrderRecorder r;
        interpFn(&r);
        ReleaseAssert(r.order == expectedAns);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        CtorDtorOrderRecorder r;
        interpFn(&r);
        ReleaseAssert(r.order == expectedAns);
    }

    thread_pochiVMContext->m_curModule->EmitIR();
    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        CtorDtorOrderRecorder r;
        jitFn(&r);
        ReleaseAssert(r.order == expectedAns);
    }
}

TEST(SanityCallCppFn, DestructorInteractionWithIfStatement_1)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(CtorDtorOrderRecorder*, bool, bool);
    {
        auto [fn, r, b1, b2] = NewFunction<FnPrototype>("testfn");
        auto v1 = fn.NewVariable<TestDestructor2>();
        auto v2 = fn.NewVariable<TestDestructor2>();
        auto v3 = fn.NewVariable<TestDestructor2>();
        auto v4 = fn.NewVariable<TestDestructor2>();
        auto v5 = fn.NewVariable<TestDestructor2>();
        auto v6 = fn.NewVariable<TestDestructor2>();
        auto v7 = fn.NewVariable<TestDestructor2>();
        auto v8 = fn.NewVariable<TestDestructor2>();
        auto v9 = fn.NewVariable<TestDestructor2>();
        auto v10 = fn.NewVariable<TestDestructor2>();
        auto v11 = fn.NewVariable<TestDestructor2>();
        auto v12 = fn.NewVariable<TestDestructor2>();
        auto v13 = fn.NewVariable<TestDestructor2>();
        auto v14 = fn.NewVariable<TestDestructor2>();
        fn.SetBody(
                r->Push(Literal<int>(1000)),
                Declare(v1, Constructor<TestDestructor2>(r, Literal<int>(1))),
                r->Push(Literal<int>(1001)),
                If(b1).Then(
                    r->Push(Literal<int>(1002)),
                    Declare(v2, Constructor<TestDestructor2>(r, Literal<int>(2))),
                    r->Push(Literal<int>(1003)),
                    If(b2).Then(
                        r->Push(Literal<int>(1004)),
                        Declare(v3, Constructor<TestDestructor2>(r, Literal<int>(3))),
                        r->Push(Literal<int>(1005)),
                        Declare(v4, Constructor<TestDestructor2>(r, Literal<int>(4))),
                        r->Push(Literal<int>(1006))
                    ).Else(
                        r->Push(Literal<int>(1007)),
                        Declare(v5, Constructor<TestDestructor2>(r, Literal<int>(5))),
                        r->Push(Literal<int>(1008)),
                        Declare(v6, Constructor<TestDestructor2>(r, Literal<int>(6))),
                        r->Push(Literal<int>(1009))
                    ),
                    r->Push(Literal<int>(1010)),
                    Declare(v7, Constructor<TestDestructor2>(r, Literal<int>(7))),
                    r->Push(Literal<int>(1011))
                ).Else(
                    r->Push(Literal<int>(1012)),
                    Declare(v8, Constructor<TestDestructor2>(r, Literal<int>(8))),
                    r->Push(Literal<int>(1013)),
                    If(b2).Then(
                        r->Push(Literal<int>(1014)),
                        Declare(v9, Constructor<TestDestructor2>(r, Literal<int>(9))),
                        r->Push(Literal<int>(1015)),
                        Declare(v10, Constructor<TestDestructor2>(r, Literal<int>(10))),
                        r->Push(Literal<int>(1016))
                    ).Else(
                        r->Push(Literal<int>(1017)),
                        Declare(v11, Constructor<TestDestructor2>(r, Literal<int>(11))),
                        r->Push(Literal<int>(1018)),
                        Declare(v12, Constructor<TestDestructor2>(r, Literal<int>(12))),
                        r->Push(Literal<int>(1019))
                    ),
                    r->Push(Literal<int>(1020)),
                    Declare(v13, Constructor<TestDestructor2>(r, Literal<int>(13))),
                    r->Push(Literal<int>(1021))
                ),
                r->Push(Literal<int>(1022)),
                Declare(v14, Constructor<TestDestructor2>(r, Literal<int>(14))),
                r->Push(Literal<int>(1023))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    std::vector<int> expectedAns1 {
            1000, 1, 1001, 1002, 2, 1003, 1004, 3, 1005, 4, 1006, -4, -3,
            1010, 7, 1011, -7, -2, 1022, 14, 1023, -14, -1
    };
    std::vector<int> expectedAns2 {
            1000, 1, 1001, 1002, 2, 1003, 1007, 5, 1008, 6, 1009, -6, -5,
            1010, 7, 1011, -7, -2, 1022, 14, 1023, -14, -1
    };
    std::vector<int> expectedAns3 {
            1000, 1, 1001, 1012, 8, 1013, 1014, 9, 1015, 10, 1016, -10, -9,
            1020, 13, 1021, -13, -8, 1022, 14, 1023, -14, -1
    };
    std::vector<int> expectedAns4 {
            1000, 1, 1001, 1012, 8, 1013, 1017, 11, 1018, 12, 1019, -12, -11,
            1020, 13, 1021, -13, -8, 1022, 14, 1023, -14, -1
    };

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            interpFn(&r, true, true);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, true, false);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, false, true);
            ReleaseAssert(r.order == expectedAns3);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, false, false);
            ReleaseAssert(r.order == expectedAns4);
        }
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            interpFn(&r, true, true);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, true, false);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, false, true);
            ReleaseAssert(r.order == expectedAns3);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, false, false);
            ReleaseAssert(r.order == expectedAns4);
        }
    }

    thread_pochiVMContext->m_curModule->EmitIR();
    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            jitFn(&r, true, true);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, true, false);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, false, true);
            ReleaseAssert(r.order == expectedAns3);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, false, false);
            ReleaseAssert(r.order == expectedAns4);
        }
    }
}

TEST(SanityCallCppFn, DestructorInteractionWithIfStatement_2)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(CtorDtorOrderRecorder*, bool, bool);
    {
        auto [fn, r, b1, b2] = NewFunction<FnPrototype>("testfn");
        auto v1 = fn.NewVariable<TestDestructor2>();
        auto v2 = fn.NewVariable<TestDestructor2>();
        auto v3 = fn.NewVariable<TestDestructor2>();
        auto v4 = fn.NewVariable<TestDestructor2>();
        auto v5 = fn.NewVariable<TestDestructor2>();
        auto v6 = fn.NewVariable<TestDestructor2>();
        auto v7 = fn.NewVariable<TestDestructor2>();
        auto v8 = fn.NewVariable<TestDestructor2>();
        auto v9 = fn.NewVariable<TestDestructor2>();
        auto v10 = fn.NewVariable<TestDestructor2>();
        auto v11 = fn.NewVariable<TestDestructor2>();
        auto v12 = fn.NewVariable<TestDestructor2>();
        auto v13 = fn.NewVariable<TestDestructor2>();
        auto v14 = fn.NewVariable<TestDestructor2>();
        fn.SetBody(
                r->Push(Literal<int>(1000)),
                Declare(v1, Constructor<TestDestructor2>(r, Literal<int>(1))),
                r->Push(Literal<int>(1001)),
                If(b1).Then(
                    r->Push(Literal<int>(1002)),
                    Declare(v2, Constructor<TestDestructor2>(r, Literal<int>(2))),
                    r->Push(Literal<int>(1003)),
                    If(b2).Then(
                        r->Push(Literal<int>(1004)),
                        Declare(v3, Constructor<TestDestructor2>(r, Literal<int>(3))),
                        r->Push(Literal<int>(1005)),
                        Declare(v4, Constructor<TestDestructor2>(r, Literal<int>(4))),
                        r->Push(Literal<int>(1006)),
                        Return()
                    ).Else(
                        r->Push(Literal<int>(1007)),
                        Declare(v5, Constructor<TestDestructor2>(r, Literal<int>(5))),
                        r->Push(Literal<int>(1008)),
                        Declare(v6, Constructor<TestDestructor2>(r, Literal<int>(6))),
                        r->Push(Literal<int>(1009))
                    ),
                    r->Push(Literal<int>(1010)),
                    Declare(v7, Constructor<TestDestructor2>(r, Literal<int>(7))),
                    r->Push(Literal<int>(1011)),
                    Return()
                ).Else(
                    r->Push(Literal<int>(1012)),
                    Declare(v8, Constructor<TestDestructor2>(r, Literal<int>(8))),
                    r->Push(Literal<int>(1013)),
                    If(b2).Then(
                        r->Push(Literal<int>(1014)),
                        Declare(v9, Constructor<TestDestructor2>(r, Literal<int>(9))),
                        r->Push(Literal<int>(1015)),
                        Declare(v10, Constructor<TestDestructor2>(r, Literal<int>(10))),
                        r->Push(Literal<int>(1016)),
                        Return()
                    ).Else(
                        r->Push(Literal<int>(1017)),
                        Declare(v11, Constructor<TestDestructor2>(r, Literal<int>(11))),
                        r->Push(Literal<int>(1018)),
                        Declare(v12, Constructor<TestDestructor2>(r, Literal<int>(12))),
                        r->Push(Literal<int>(1019))
                    ),
                    r->Push(Literal<int>(1020)),
                    Declare(v13, Constructor<TestDestructor2>(r, Literal<int>(13))),
                    r->Push(Literal<int>(1021))
                ),
                r->Push(Literal<int>(1022)),
                Declare(v14, Constructor<TestDestructor2>(r, Literal<int>(14))),
                r->Push(Literal<int>(1023))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    std::vector<int> expectedAns1 {
            1000, 1, 1001, 1002, 2, 1003, 1004, 3, 1005, 4, 1006, -4, -3, -2, -1
    };
    std::vector<int> expectedAns2 {
            1000, 1, 1001, 1002, 2, 1003, 1007, 5, 1008, 6, 1009, -6, -5,
            1010, 7, 1011, -7, -2, -1
    };
    std::vector<int> expectedAns3 {
            1000, 1, 1001, 1012, 8, 1013, 1014, 9, 1015, 10, 1016, -10, -9, -8, -1
    };
    std::vector<int> expectedAns4 {
            1000, 1, 1001, 1012, 8, 1013, 1017, 11, 1018, 12, 1019, -12, -11,
            1020, 13, 1021, -13, -8, 1022, 14, 1023, -14, -1
    };

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            interpFn(&r, true, true);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, true, false);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, false, true);
            ReleaseAssert(r.order == expectedAns3);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, false, false);
            ReleaseAssert(r.order == expectedAns4);
        }
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            interpFn(&r, true, true);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, true, false);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, false, true);
            ReleaseAssert(r.order == expectedAns3);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, false, false);
            ReleaseAssert(r.order == expectedAns4);
        }
    }


    thread_pochiVMContext->m_curModule->EmitIR();
    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            jitFn(&r, true, true);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, true, false);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, false, true);
            ReleaseAssert(r.order == expectedAns3);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, false, false);
            ReleaseAssert(r.order == expectedAns4);
        }
    }
}

TEST(SanityCallCppFn, DestructorInteractionWithIfStatement_3)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(CtorDtorOrderRecorder*, bool, bool);
    {
        auto [fn, r, b1, b2] = NewFunction<FnPrototype>("testfn");
        auto v1 = fn.NewVariable<TestDestructor2>();
        auto v2 = fn.NewVariable<TestDestructor2>();
        auto v3 = fn.NewVariable<TestDestructor2>();
        auto v4 = fn.NewVariable<TestDestructor2>();
        auto v5 = fn.NewVariable<TestDestructor2>();
        auto v6 = fn.NewVariable<TestDestructor2>();
        auto v8 = fn.NewVariable<TestDestructor2>();
        auto v9 = fn.NewVariable<TestDestructor2>();
        auto v10 = fn.NewVariable<TestDestructor2>();
        auto v11 = fn.NewVariable<TestDestructor2>();
        auto v12 = fn.NewVariable<TestDestructor2>();
        auto v13 = fn.NewVariable<TestDestructor2>();
        fn.SetBody(
                r->Push(Literal<int>(1000)),
                Declare(v1, Constructor<TestDestructor2>(r, Literal<int>(1))),
                r->Push(Literal<int>(1001)),
                If(b1).Then(
                    r->Push(Literal<int>(1002)),
                    Declare(v2, Constructor<TestDestructor2>(r, Literal<int>(2))),
                    r->Push(Literal<int>(1003)),
                    If(b2).Then(
                        r->Push(Literal<int>(1004)),
                        Declare(v3, Constructor<TestDestructor2>(r, Literal<int>(3))),
                        r->Push(Literal<int>(1005)),
                        Declare(v4, Constructor<TestDestructor2>(r, Literal<int>(4))),
                        r->Push(Literal<int>(1006)),
                        Return()
                    ).Else(
                        r->Push(Literal<int>(1007)),
                        Declare(v5, Constructor<TestDestructor2>(r, Literal<int>(5))),
                        r->Push(Literal<int>(1008)),
                        Declare(v6, Constructor<TestDestructor2>(r, Literal<int>(6))),
                        r->Push(Literal<int>(1009)),
                        Return()
                    )
                ).Else(
                    r->Push(Literal<int>(1012)),
                    Declare(v8, Constructor<TestDestructor2>(r, Literal<int>(8))),
                    r->Push(Literal<int>(1013)),
                    If(b2).Then(
                        r->Push(Literal<int>(1014)),
                        Declare(v9, Constructor<TestDestructor2>(r, Literal<int>(9))),
                        r->Push(Literal<int>(1015)),
                        Declare(v10, Constructor<TestDestructor2>(r, Literal<int>(10))),
                        r->Push(Literal<int>(1016)),
                        Return()
                    ).Else(
                        r->Push(Literal<int>(1017)),
                        Declare(v11, Constructor<TestDestructor2>(r, Literal<int>(11))),
                        r->Push(Literal<int>(1018)),
                        Declare(v12, Constructor<TestDestructor2>(r, Literal<int>(12))),
                        r->Push(Literal<int>(1019))
                    ),
                    r->Push(Literal<int>(1020)),
                    Declare(v13, Constructor<TestDestructor2>(r, Literal<int>(13))),
                    r->Push(Literal<int>(1021)),
                    Return()
                )
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    std::vector<int> expectedAns1 {
            1000, 1, 1001, 1002, 2, 1003, 1004, 3, 1005, 4, 1006, -4, -3, -2, -1
    };
    std::vector<int> expectedAns2 {
            1000, 1, 1001, 1002, 2, 1003, 1007, 5, 1008, 6, 1009, -6, -5, -2, -1
    };
    std::vector<int> expectedAns3 {
            1000, 1, 1001, 1012, 8, 1013, 1014, 9, 1015, 10, 1016, -10, -9, -8, -1
    };
    std::vector<int> expectedAns4 {
            1000, 1, 1001, 1012, 8, 1013, 1017, 11, 1018, 12, 1019, -12, -11,
            1020, 13, 1021, -13, -8, -1
    };

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            interpFn(&r, true, true);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, true, false);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, false, true);
            ReleaseAssert(r.order == expectedAns3);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, false, false);
            ReleaseAssert(r.order == expectedAns4);
        }
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            interpFn(&r, true, true);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, true, false);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, false, true);
            ReleaseAssert(r.order == expectedAns3);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, false, false);
            ReleaseAssert(r.order == expectedAns4);
        }
    }

    thread_pochiVMContext->m_curModule->EmitIR();
    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            jitFn(&r, true, true);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, true, false);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, false, true);
            ReleaseAssert(r.order == expectedAns3);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, false, false);
            ReleaseAssert(r.order == expectedAns4);
        }
    }
}

TEST(SanityCallCppFn, DestructorInteractionWithForLoop_1)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(CtorDtorOrderRecorder*, int);
    {
        auto [fn, r, x] = NewFunction<FnPrototype>("testfn");
        auto i = fn.NewVariable<int>();
        auto v1 = fn.NewVariable<TestDestructor2>();
        auto v2 = fn.NewVariable<TestDestructor2>();
        auto v3 = fn.NewVariable<TestDestructor2>();
        auto v4 = fn.NewVariable<TestDestructor2>();
        auto v5 = fn.NewVariable<TestDestructor2>();
        auto v6 = fn.NewVariable<TestDestructor2>();
        auto v7 = fn.NewVariable<TestDestructor2>();
        auto v8 = fn.NewVariable<TestDestructor2>();
        auto v9 = fn.NewVariable<TestDestructor2>();
        auto v10 = fn.NewVariable<TestDestructor2>();
        auto v11 = fn.NewVariable<TestDestructor2>();
        auto v12 = fn.NewVariable<TestDestructor2>();
        fn.SetBody(
                r->Push(Literal<int>(1000)),
                Declare(v1, Constructor<TestDestructor2>(r, Literal<int>(1))),
                r->Push(Literal<int>(1001)),
                For(Block(
                    r->Push(Literal<int>(1002)),
                    Declare(v2, Constructor<TestDestructor2>(r, Literal<int>(2))),
                    r->Push(Literal<int>(1003)),
                    Declare(v3, Constructor<TestDestructor2>(r, Literal<int>(3))),
                    r->Push(Literal<int>(1004)),
                    Declare(i, Literal<int>(0))
                ), i < Literal<int>(3), Assign(i, i + Literal<int>(1))).Do(
                    r->Push(Literal<int>(1005)),
                    Declare(v4, Constructor<TestDestructor2>(r, Literal<int>(4))),
                    r->Push(Literal<int>(1006)),
                    Declare(v5, Constructor<TestDestructor2>(r, Literal<int>(5))),
                    r->Push(Literal<int>(1007)),
                    If(i == x).Then(
                        r->Push(Literal<int>(1008)),
                        Declare(v6, Constructor<TestDestructor2>(r, Literal<int>(6))),
                        r->Push(Literal<int>(1009)),
                        Declare(v7, Constructor<TestDestructor2>(r, Literal<int>(7))),
                        r->Push(Literal<int>(1010)),
                        Break()
                    ).Else(
                        r->Push(Literal<int>(1011)),
                        Declare(v8, Constructor<TestDestructor2>(r, Literal<int>(8))),
                        r->Push(Literal<int>(1012)),
                        Declare(v9, Constructor<TestDestructor2>(r, Literal<int>(9))),
                        r->Push(Literal<int>(1013))
                    ),
                    r->Push(Literal<int>(1014)),
                    Declare(v10, Constructor<TestDestructor2>(r, Literal<int>(10))),
                    r->Push(Literal<int>(1015)),
                    Declare(v11, Constructor<TestDestructor2>(r, Literal<int>(11))),
                    r->Push(Literal<int>(1016))
                ),
                r->Push(Literal<int>(1017)),
                Declare(v12, Constructor<TestDestructor2>(r, Literal<int>(12))),
                r->Push(Literal<int>(1018))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    std::vector<int> expectedAns0 {
            1000, 1, 1001, 1002, 2, 1003, 3, 1004,
            1005, 4, 1006, 5, 1007, 1008, 6, 1009, 7, 1010, -7, -6, -5, -4,
            -3, -2, 1017, 12, 1018, -12, -1
    };
    std::vector<int> expectedAns1 {
            1000, 1, 1001, 1002, 2, 1003, 3, 1004,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1008, 6, 1009, 7, 1010, -7, -6, -5, -4,
            -3, -2, 1017, 12, 1018, -12, -1
    };
    std::vector<int> expectedAns2 {
            1000, 1, 1001, 1002, 2, 1003, 3, 1004,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1008, 6, 1009, 7, 1010, -7, -6, -5, -4,
            -3, -2, 1017, 12, 1018, -12, -1
    };
    std::vector<int> expectedAns3 {
            1000, 1, 1001, 1002, 2, 1003, 3, 1004,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            -3, -2, 1017, 12, 1018, -12, -1
    };

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 0);
            ReleaseAssert(r.order == expectedAns0);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 1);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 2);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 3);
            ReleaseAssert(r.order == expectedAns3);
        }
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 0);
            ReleaseAssert(r.order == expectedAns0);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 1);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 2);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 3);
            ReleaseAssert(r.order == expectedAns3);
        }
    }

    thread_pochiVMContext->m_curModule->EmitIR();
    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            jitFn(&r, 0);
            ReleaseAssert(r.order == expectedAns0);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, 1);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, 2);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, 3);
            ReleaseAssert(r.order == expectedAns3);
        }
    }
}

TEST(SanityCallCppFn, DestructorInteractionWithForLoop_2)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(CtorDtorOrderRecorder*, int);
    {
        auto [fn, r, x] = NewFunction<FnPrototype>("testfn");
        auto i = fn.NewVariable<int>();
        auto v1 = fn.NewVariable<TestDestructor2>();
        auto v2 = fn.NewVariable<TestDestructor2>();
        auto v3 = fn.NewVariable<TestDestructor2>();
        auto v4 = fn.NewVariable<TestDestructor2>();
        auto v5 = fn.NewVariable<TestDestructor2>();
        auto v6 = fn.NewVariable<TestDestructor2>();
        auto v7 = fn.NewVariable<TestDestructor2>();
        auto v8 = fn.NewVariable<TestDestructor2>();
        auto v9 = fn.NewVariable<TestDestructor2>();
        auto v10 = fn.NewVariable<TestDestructor2>();
        auto v11 = fn.NewVariable<TestDestructor2>();
        auto v12 = fn.NewVariable<TestDestructor2>();
        fn.SetBody(
                r->Push(Literal<int>(1000)),
                Declare(v1, Constructor<TestDestructor2>(r, Literal<int>(1))),
                r->Push(Literal<int>(1001)),
                For(Block(
                    r->Push(Literal<int>(1002)),
                    Declare(v2, Constructor<TestDestructor2>(r, Literal<int>(2))),
                    r->Push(Literal<int>(1003)),
                    Declare(v3, Constructor<TestDestructor2>(r, Literal<int>(3))),
                    r->Push(Literal<int>(1004)),
                    Declare(i, Literal<int>(0))
                ), i < Literal<int>(3), Assign(i, i + Literal<int>(1))).Do(
                    r->Push(Literal<int>(1005)),
                    Declare(v4, Constructor<TestDestructor2>(r, Literal<int>(4))),
                    r->Push(Literal<int>(1006)),
                    Declare(v5, Constructor<TestDestructor2>(r, Literal<int>(5))),
                    r->Push(Literal<int>(1007)),
                    If(i == x).Then(
                        r->Push(Literal<int>(1008)),
                        Declare(v6, Constructor<TestDestructor2>(r, Literal<int>(6))),
                        r->Push(Literal<int>(1009)),
                        Declare(v7, Constructor<TestDestructor2>(r, Literal<int>(7))),
                        r->Push(Literal<int>(1010)),
                        Continue()
                    ).Else(
                        r->Push(Literal<int>(1011)),
                        Declare(v8, Constructor<TestDestructor2>(r, Literal<int>(8))),
                        r->Push(Literal<int>(1012)),
                        Declare(v9, Constructor<TestDestructor2>(r, Literal<int>(9))),
                        r->Push(Literal<int>(1013))
                    ),
                    r->Push(Literal<int>(1014)),
                    Declare(v10, Constructor<TestDestructor2>(r, Literal<int>(10))),
                    r->Push(Literal<int>(1015)),
                    Declare(v11, Constructor<TestDestructor2>(r, Literal<int>(11))),
                    r->Push(Literal<int>(1016))
                ),
                r->Push(Literal<int>(1017)),
                Declare(v12, Constructor<TestDestructor2>(r, Literal<int>(12))),
                r->Push(Literal<int>(1018))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    std::vector<int> expectedAns0 {
            1000, 1, 1001, 1002, 2, 1003, 3, 1004,
            1005, 4, 1006, 5, 1007, 1008, 6, 1009, 7, 1010, -7, -6, -5, -4,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            -3, -2, 1017, 12, 1018, -12, -1
    };
    std::vector<int> expectedAns1 {
            1000, 1, 1001, 1002, 2, 1003, 3, 1004,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1008, 6, 1009, 7, 1010, -7, -6, -5, -4,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            -3, -2, 1017, 12, 1018, -12, -1
    };
    std::vector<int> expectedAns2 {
            1000, 1, 1001, 1002, 2, 1003, 3, 1004,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1008, 6, 1009, 7, 1010, -7, -6, -5, -4,
            -3, -2, 1017, 12, 1018, -12, -1
    };
    std::vector<int> expectedAns3 {
            1000, 1, 1001, 1002, 2, 1003, 3, 1004,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            -3, -2, 1017, 12, 1018, -12, -1
    };

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 0);
            ReleaseAssert(r.order == expectedAns0);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 1);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 2);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 3);
            ReleaseAssert(r.order == expectedAns3);
        }
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 0);
            ReleaseAssert(r.order == expectedAns0);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 1);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 2);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 3);
            ReleaseAssert(r.order == expectedAns3);
        }
    }

    thread_pochiVMContext->m_curModule->EmitIR();
    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            jitFn(&r, 0);
            ReleaseAssert(r.order == expectedAns0);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, 1);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, 2);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, 3);
            ReleaseAssert(r.order == expectedAns3);
        }
    }
}

TEST(SanityCallCppFn, DestructorInteractionWithForLoop_3)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(CtorDtorOrderRecorder*, int);
    {
        auto [fn, r, x] = NewFunction<FnPrototype>("testfn");
        auto i = fn.NewVariable<int>();
        auto v1 = fn.NewVariable<TestDestructor2>();
        auto v2 = fn.NewVariable<TestDestructor2>();
        auto v3 = fn.NewVariable<TestDestructor2>();
        auto v4 = fn.NewVariable<TestDestructor2>();
        auto v5 = fn.NewVariable<TestDestructor2>();
        auto v6 = fn.NewVariable<TestDestructor2>();
        auto v7 = fn.NewVariable<TestDestructor2>();
        auto v8 = fn.NewVariable<TestDestructor2>();
        auto v9 = fn.NewVariable<TestDestructor2>();
        auto v10 = fn.NewVariable<TestDestructor2>();
        auto v11 = fn.NewVariable<TestDestructor2>();
        auto v12 = fn.NewVariable<TestDestructor2>();
        fn.SetBody(
                r->Push(Literal<int>(1000)),
                Declare(v1, Constructor<TestDestructor2>(r, Literal<int>(1))),
                r->Push(Literal<int>(1001)),
                For(Block(
                    r->Push(Literal<int>(1002)),
                    Declare(v2, Constructor<TestDestructor2>(r, Literal<int>(2))),
                    r->Push(Literal<int>(1003)),
                    Declare(v3, Constructor<TestDestructor2>(r, Literal<int>(3))),
                    r->Push(Literal<int>(1004)),
                    Declare(i, Literal<int>(0))
                ), i < Literal<int>(3), Assign(i, i + Literal<int>(1))).Do(
                    r->Push(Literal<int>(1005)),
                    Declare(v4, Constructor<TestDestructor2>(r, Literal<int>(4))),
                    r->Push(Literal<int>(1006)),
                    Declare(v5, Constructor<TestDestructor2>(r, Literal<int>(5))),
                    r->Push(Literal<int>(1007)),
                    If(i == x).Then(
                        r->Push(Literal<int>(1008)),
                        Declare(v6, Constructor<TestDestructor2>(r, Literal<int>(6))),
                        r->Push(Literal<int>(1009)),
                        Declare(v7, Constructor<TestDestructor2>(r, Literal<int>(7))),
                        r->Push(Literal<int>(1010)),
                        Return()
                    ).Else(
                        r->Push(Literal<int>(1011)),
                        Declare(v8, Constructor<TestDestructor2>(r, Literal<int>(8))),
                        r->Push(Literal<int>(1012)),
                        Declare(v9, Constructor<TestDestructor2>(r, Literal<int>(9))),
                        r->Push(Literal<int>(1013))
                    ),
                    r->Push(Literal<int>(1014)),
                    Declare(v10, Constructor<TestDestructor2>(r, Literal<int>(10))),
                    r->Push(Literal<int>(1015)),
                    Declare(v11, Constructor<TestDestructor2>(r, Literal<int>(11))),
                    r->Push(Literal<int>(1016))
                ),
                r->Push(Literal<int>(1017)),
                Declare(v12, Constructor<TestDestructor2>(r, Literal<int>(12))),
                r->Push(Literal<int>(1018))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    std::vector<int> expectedAns0 {
            1000, 1, 1001, 1002, 2, 1003, 3, 1004,
            1005, 4, 1006, 5, 1007, 1008, 6, 1009, 7, 1010, -7, -6, -5, -4,
            -3, -2, -1
    };
    std::vector<int> expectedAns1 {
            1000, 1, 1001, 1002, 2, 1003, 3, 1004,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1008, 6, 1009, 7, 1010, -7, -6, -5, -4,
            -3, -2, -1
    };
    std::vector<int> expectedAns2 {
            1000, 1, 1001, 1002, 2, 1003, 3, 1004,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1008, 6, 1009, 7, 1010, -7, -6, -5, -4,
            -3, -2, -1
    };
    std::vector<int> expectedAns3 {
            1000, 1, 1001, 1002, 2, 1003, 3, 1004,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            -3, -2, 1017, 12, 1018, -12, -1
    };

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 0);
            ReleaseAssert(r.order == expectedAns0);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 1);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 2);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 3);
            ReleaseAssert(r.order == expectedAns3);
        }
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 0);
            ReleaseAssert(r.order == expectedAns0);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 1);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 2);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 3);
            ReleaseAssert(r.order == expectedAns3);
        }
    }

    thread_pochiVMContext->m_curModule->EmitIR();
    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            jitFn(&r, 0);
            ReleaseAssert(r.order == expectedAns0);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, 1);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, 2);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, 3);
            ReleaseAssert(r.order == expectedAns3);
        }
    }
}

TEST(SanityCallCppFn, DestructorInteractionWithForLoop_4)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(CtorDtorOrderRecorder*, bool);
    {
        auto [fn, r, b] = NewFunction<FnPrototype>("testfn");
        auto v1 = fn.NewVariable<TestDestructor2>();
        auto v2 = fn.NewVariable<TestDestructor2>();
        auto v3 = fn.NewVariable<TestDestructor2>();
        auto v4 = fn.NewVariable<TestDestructor2>();
        auto v5 = fn.NewVariable<TestDestructor2>();
        auto v6 = fn.NewVariable<TestDestructor2>();
        fn.SetBody(
                r->Push(Literal<int>(1000)),
                Declare(v1, Constructor<TestDestructor2>(r, Literal<int>(1))),
                r->Push(Literal<int>(1001)),
                For(Block(
                    r->Push(Literal<int>(1002)),
                    Declare(v2, Constructor<TestDestructor2>(r, Literal<int>(2))),
                    r->Push(Literal<int>(1003)),
                    Declare(v3, Constructor<TestDestructor2>(r, Literal<int>(3))),
                    r->Push(Literal<int>(1004))
                ), b, Block()).Do(
                    r->Push(Literal<int>(1005)),
                    Declare(v4, Constructor<TestDestructor2>(r, Literal<int>(4))),
                    r->Push(Literal<int>(1006)),
                    Declare(v5, Constructor<TestDestructor2>(r, Literal<int>(5))),
                    r->Push(Literal<int>(1007)),
                    Break()
                ),
                r->Push(Literal<int>(1008)),
                Declare(v6, Constructor<TestDestructor2>(r, Literal<int>(6))),
                r->Push(Literal<int>(1009))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    std::vector<int> expectedAns1 {
            1000, 1, 1001, 1002, 2, 1003, 3, 1004,
            1005, 4, 1006, 5, 1007, -5, -4, -3, -2, 1008, 6, 1009, -6, -1
    };
    std::vector<int> expectedAns2 {
            1000, 1, 1001, 1002, 2, 1003, 3, 1004,
            -3, -2, 1008, 6, 1009, -6, -1
    };

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            interpFn(&r, true);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, false);
            ReleaseAssert(r.order == expectedAns2);
        }
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            interpFn(&r, true);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, false);
            ReleaseAssert(r.order == expectedAns2);
        }
    }

    thread_pochiVMContext->m_curModule->EmitIR();
    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            jitFn(&r, true);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, false);
            ReleaseAssert(r.order == expectedAns2);
        }
    }
}

TEST(SanityCallCppFn, DestructorInteractionWithWhileLoop_1)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(CtorDtorOrderRecorder*, int);
    {
        auto [fn, r, x] = NewFunction<FnPrototype>("testfn");
        auto i = fn.NewVariable<int>();
        auto v1 = fn.NewVariable<TestDestructor2>();
        auto v2 = fn.NewVariable<TestDestructor2>();
        auto v4 = fn.NewVariable<TestDestructor2>();
        auto v5 = fn.NewVariable<TestDestructor2>();
        auto v6 = fn.NewVariable<TestDestructor2>();
        auto v7 = fn.NewVariable<TestDestructor2>();
        auto v8 = fn.NewVariable<TestDestructor2>();
        auto v9 = fn.NewVariable<TestDestructor2>();
        auto v10 = fn.NewVariable<TestDestructor2>();
        auto v11 = fn.NewVariable<TestDestructor2>();
        auto v12 = fn.NewVariable<TestDestructor2>();
        fn.SetBody(
                r->Push(Literal<int>(1000)),
                Declare(v1, Constructor<TestDestructor2>(r, Literal<int>(1))),
                r->Push(Literal<int>(1001)),
                Declare(v2, Constructor<TestDestructor2>(r, Literal<int>(2))),
                r->Push(Literal<int>(1002)),
                Declare(i, Literal<int>(0)),
                While(i < Literal<int>(3)).Do(
                    Assign(i, i + Literal<int>(1)),
                    r->Push(Literal<int>(1005)),
                    Declare(v4, Constructor<TestDestructor2>(r, Literal<int>(4))),
                    r->Push(Literal<int>(1006)),
                    Declare(v5, Constructor<TestDestructor2>(r, Literal<int>(5))),
                    r->Push(Literal<int>(1007)),
                    If(i - Literal<int>(1) == x).Then(
                        r->Push(Literal<int>(1008)),
                        Declare(v6, Constructor<TestDestructor2>(r, Literal<int>(6))),
                        r->Push(Literal<int>(1009)),
                        Declare(v7, Constructor<TestDestructor2>(r, Literal<int>(7))),
                        r->Push(Literal<int>(1010)),
                        Return()
                    ).Else(
                        r->Push(Literal<int>(1011)),
                        Declare(v8, Constructor<TestDestructor2>(r, Literal<int>(8))),
                        r->Push(Literal<int>(1012)),
                        Declare(v9, Constructor<TestDestructor2>(r, Literal<int>(9))),
                        r->Push(Literal<int>(1013))
                    ),
                    r->Push(Literal<int>(1014)),
                    Declare(v10, Constructor<TestDestructor2>(r, Literal<int>(10))),
                    r->Push(Literal<int>(1015)),
                    Declare(v11, Constructor<TestDestructor2>(r, Literal<int>(11))),
                    r->Push(Literal<int>(1016))
                ),
                r->Push(Literal<int>(1017)),
                Declare(v12, Constructor<TestDestructor2>(r, Literal<int>(12))),
                r->Push(Literal<int>(1018))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    std::vector<int> expectedAns0 {
            1000, 1, 1001, 2, 1002,
            1005, 4, 1006, 5, 1007, 1008, 6, 1009, 7, 1010, -7, -6, -5, -4,
            -2, -1
    };
    std::vector<int> expectedAns1 {
            1000, 1, 1001, 2, 1002,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1008, 6, 1009, 7, 1010, -7, -6, -5, -4,
            -2, -1
    };
    std::vector<int> expectedAns2 {
            1000, 1, 1001, 2, 1002,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1008, 6, 1009, 7, 1010, -7, -6, -5, -4,
            -2, -1
    };
    std::vector<int> expectedAns3 {
            1000, 1, 1001, 2, 1002,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1017, 12, 1018, -12, -2, -1
    };

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 0);
            ReleaseAssert(r.order == expectedAns0);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 1);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 2);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 3);
            ReleaseAssert(r.order == expectedAns3);
        }
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 0);
            ReleaseAssert(r.order == expectedAns0);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 1);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 2);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 3);
            ReleaseAssert(r.order == expectedAns3);
        }
    }

    thread_pochiVMContext->m_curModule->EmitIR();
    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            jitFn(&r, 0);
            ReleaseAssert(r.order == expectedAns0);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, 1);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, 2);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, 3);
            ReleaseAssert(r.order == expectedAns3);
        }
    }
}

TEST(SanityCallCppFn, DestructorInteractionWithWhileLoop_2)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(CtorDtorOrderRecorder*, int);
    {
        auto [fn, r, x] = NewFunction<FnPrototype>("testfn");
        auto i = fn.NewVariable<int>();
        auto v1 = fn.NewVariable<TestDestructor2>();
        auto v2 = fn.NewVariable<TestDestructor2>();
        auto v4 = fn.NewVariable<TestDestructor2>();
        auto v5 = fn.NewVariable<TestDestructor2>();
        auto v6 = fn.NewVariable<TestDestructor2>();
        auto v7 = fn.NewVariable<TestDestructor2>();
        auto v8 = fn.NewVariable<TestDestructor2>();
        auto v9 = fn.NewVariable<TestDestructor2>();
        auto v10 = fn.NewVariable<TestDestructor2>();
        auto v11 = fn.NewVariable<TestDestructor2>();
        auto v12 = fn.NewVariable<TestDestructor2>();
        fn.SetBody(
                r->Push(Literal<int>(1000)),
                Declare(v1, Constructor<TestDestructor2>(r, Literal<int>(1))),
                r->Push(Literal<int>(1001)),
                Declare(v2, Constructor<TestDestructor2>(r, Literal<int>(2))),
                r->Push(Literal<int>(1002)),
                Declare(i, Literal<int>(0)),
                While(i < Literal<int>(3)).Do(
                    Assign(i, i + Literal<int>(1)),
                    r->Push(Literal<int>(1005)),
                    Declare(v4, Constructor<TestDestructor2>(r, Literal<int>(4))),
                    r->Push(Literal<int>(1006)),
                    Declare(v5, Constructor<TestDestructor2>(r, Literal<int>(5))),
                    r->Push(Literal<int>(1007)),
                    If(i - Literal<int>(1) == x).Then(
                        r->Push(Literal<int>(1008)),
                        Declare(v6, Constructor<TestDestructor2>(r, Literal<int>(6))),
                        r->Push(Literal<int>(1009)),
                        Declare(v7, Constructor<TestDestructor2>(r, Literal<int>(7))),
                        r->Push(Literal<int>(1010)),
                        Break()
                    ).Else(
                        r->Push(Literal<int>(1011)),
                        Declare(v8, Constructor<TestDestructor2>(r, Literal<int>(8))),
                        r->Push(Literal<int>(1012)),
                        Declare(v9, Constructor<TestDestructor2>(r, Literal<int>(9))),
                        r->Push(Literal<int>(1013))
                    ),
                    r->Push(Literal<int>(1014)),
                    Declare(v10, Constructor<TestDestructor2>(r, Literal<int>(10))),
                    r->Push(Literal<int>(1015)),
                    Declare(v11, Constructor<TestDestructor2>(r, Literal<int>(11))),
                    r->Push(Literal<int>(1016))
                ),
                r->Push(Literal<int>(1017)),
                Declare(v12, Constructor<TestDestructor2>(r, Literal<int>(12))),
                r->Push(Literal<int>(1018))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    std::vector<int> expectedAns0 {
            1000, 1, 1001, 2, 1002,
            1005, 4, 1006, 5, 1007, 1008, 6, 1009, 7, 1010, -7, -6, -5, -4,
            1017, 12, 1018, -12, -2, -1
    };
    std::vector<int> expectedAns1 {
            1000, 1, 1001, 2, 1002,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1008, 6, 1009, 7, 1010, -7, -6, -5, -4,
            1017, 12, 1018, -12, -2, -1
    };
    std::vector<int> expectedAns2 {
            1000, 1, 1001, 2, 1002,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1008, 6, 1009, 7, 1010, -7, -6, -5, -4,
            1017, 12, 1018, -12, -2, -1
    };
    std::vector<int> expectedAns3 {
            1000, 1, 1001, 2, 1002,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1017, 12, 1018, -12, -2, -1
    };

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 0);
            ReleaseAssert(r.order == expectedAns0);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 1);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 2);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 3);
            ReleaseAssert(r.order == expectedAns3);
        }
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 0);
            ReleaseAssert(r.order == expectedAns0);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 1);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 2);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 3);
            ReleaseAssert(r.order == expectedAns3);
        }
    }

    thread_pochiVMContext->m_curModule->EmitIR();
    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            jitFn(&r, 0);
            ReleaseAssert(r.order == expectedAns0);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, 1);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, 2);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, 3);
            ReleaseAssert(r.order == expectedAns3);
        }
    }
}

TEST(SanityCallCppFn, DestructorInteractionWithWhileLoop_3)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(CtorDtorOrderRecorder*, int);
    {
        auto [fn, r, x] = NewFunction<FnPrototype>("testfn");
        auto i = fn.NewVariable<int>();
        auto v1 = fn.NewVariable<TestDestructor2>();
        auto v2 = fn.NewVariable<TestDestructor2>();
        auto v4 = fn.NewVariable<TestDestructor2>();
        auto v5 = fn.NewVariable<TestDestructor2>();
        auto v6 = fn.NewVariable<TestDestructor2>();
        auto v7 = fn.NewVariable<TestDestructor2>();
        auto v8 = fn.NewVariable<TestDestructor2>();
        auto v9 = fn.NewVariable<TestDestructor2>();
        auto v10 = fn.NewVariable<TestDestructor2>();
        auto v11 = fn.NewVariable<TestDestructor2>();
        auto v12 = fn.NewVariable<TestDestructor2>();
        fn.SetBody(
                r->Push(Literal<int>(1000)),
                Declare(v1, Constructor<TestDestructor2>(r, Literal<int>(1))),
                r->Push(Literal<int>(1001)),
                Declare(v2, Constructor<TestDestructor2>(r, Literal<int>(2))),
                r->Push(Literal<int>(1002)),
                Declare(i, Literal<int>(0)),
                While(i < Literal<int>(3)).Do(
                    Assign(i, i + Literal<int>(1)),
                    r->Push(Literal<int>(1005)),
                    Declare(v4, Constructor<TestDestructor2>(r, Literal<int>(4))),
                    r->Push(Literal<int>(1006)),
                    Declare(v5, Constructor<TestDestructor2>(r, Literal<int>(5))),
                    r->Push(Literal<int>(1007)),
                    If(i - Literal<int>(1) == x).Then(
                        r->Push(Literal<int>(1008)),
                        Declare(v6, Constructor<TestDestructor2>(r, Literal<int>(6))),
                        r->Push(Literal<int>(1009)),
                        Declare(v7, Constructor<TestDestructor2>(r, Literal<int>(7))),
                        r->Push(Literal<int>(1010)),
                        Continue()
                    ).Else(
                        r->Push(Literal<int>(1011)),
                        Declare(v8, Constructor<TestDestructor2>(r, Literal<int>(8))),
                        r->Push(Literal<int>(1012)),
                        Declare(v9, Constructor<TestDestructor2>(r, Literal<int>(9))),
                        r->Push(Literal<int>(1013))
                    ),
                    r->Push(Literal<int>(1014)),
                    Declare(v10, Constructor<TestDestructor2>(r, Literal<int>(10))),
                    r->Push(Literal<int>(1015)),
                    Declare(v11, Constructor<TestDestructor2>(r, Literal<int>(11))),
                    r->Push(Literal<int>(1016))
                ),
                r->Push(Literal<int>(1017)),
                Declare(v12, Constructor<TestDestructor2>(r, Literal<int>(12))),
                r->Push(Literal<int>(1018))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    std::vector<int> expectedAns0 {
            1000, 1, 1001, 2, 1002,
            1005, 4, 1006, 5, 1007, 1008, 6, 1009, 7, 1010, -7, -6, -5, -4,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1017, 12, 1018, -12, -2, -1
    };
    std::vector<int> expectedAns1 {
            1000, 1, 1001, 2, 1002,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1008, 6, 1009, 7, 1010, -7, -6, -5, -4,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1017, 12, 1018, -12, -2, -1
    };
    std::vector<int> expectedAns2 {
            1000, 1, 1001, 2, 1002,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1008, 6, 1009, 7, 1010, -7, -6, -5, -4,
            1017, 12, 1018, -12, -2, -1
    };
    std::vector<int> expectedAns3 {
            1000, 1, 1001, 2, 1002,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1005, 4, 1006, 5, 1007, 1011, 8, 1012, 9, 1013, -9, -8, 1014, 10, 1015, 11, 1016, -11, -10, -5, -4,
            1017, 12, 1018, -12, -2, -1
    };

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 0);
            ReleaseAssert(r.order == expectedAns0);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 1);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 2);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 3);
            ReleaseAssert(r.order == expectedAns3);
        }
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 0);
            ReleaseAssert(r.order == expectedAns0);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 1);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 2);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, 3);
            ReleaseAssert(r.order == expectedAns3);
        }
    }

    thread_pochiVMContext->m_curModule->EmitIR();
    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            jitFn(&r, 0);
            ReleaseAssert(r.order == expectedAns0);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, 1);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, 2);
            ReleaseAssert(r.order == expectedAns2);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, 3);
            ReleaseAssert(r.order == expectedAns3);
        }
    }
}

TEST(SanityCallCppFn, DestructorInteractionWithWhileLoop_4)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(CtorDtorOrderRecorder*, bool);
    {
        auto [fn, r, b] = NewFunction<FnPrototype>("testfn");
        auto v1 = fn.NewVariable<TestDestructor2>();
        auto v2 = fn.NewVariable<TestDestructor2>();
        auto v4 = fn.NewVariable<TestDestructor2>();
        auto v5 = fn.NewVariable<TestDestructor2>();
        auto v6 = fn.NewVariable<TestDestructor2>();
        fn.SetBody(
                r->Push(Literal<int>(1000)),
                Declare(v1, Constructor<TestDestructor2>(r, Literal<int>(1))),
                r->Push(Literal<int>(1001)),
                Declare(v2, Constructor<TestDestructor2>(r, Literal<int>(2))),
                r->Push(Literal<int>(1002)),
                While(b).Do(
                    r->Push(Literal<int>(1005)),
                    Declare(v4, Constructor<TestDestructor2>(r, Literal<int>(4))),
                    r->Push(Literal<int>(1006)),
                    Declare(v5, Constructor<TestDestructor2>(r, Literal<int>(5))),
                    r->Push(Literal<int>(1007)),
                    Break()
                ),
                r->Push(Literal<int>(1008)),
                Declare(v6, Constructor<TestDestructor2>(r, Literal<int>(6))),
                r->Push(Literal<int>(1009))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    std::vector<int> expectedAns1 {
            1000, 1, 1001, 2, 1002,
            1005, 4, 1006, 5, 1007, -5, -4, 1008, 6, 1009, -6, -2, -1
    };
    std::vector<int> expectedAns2 {
            1000, 1, 1001, 2, 1002, 1008, 6, 1009, -6, -2, -1
    };

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            interpFn(&r, true);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, false);
            ReleaseAssert(r.order == expectedAns2);
        }
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            interpFn(&r, true);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            interpFn(&r, false);
            ReleaseAssert(r.order == expectedAns2);
        }
    }

    thread_pochiVMContext->m_curModule->EmitIR();
    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        {
            CtorDtorOrderRecorder r;
            jitFn(&r, true);
            ReleaseAssert(r.order == expectedAns1);
        }
        {
            CtorDtorOrderRecorder r;
            jitFn(&r, false);
            ReleaseAssert(r.order == expectedAns2);
        }
    }
}

TEST(SanityCallCppFn, StaticVarInFunction)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)();
    {
        auto [fn] = NewFunction<FnPrototype>("testfn");
        fn.SetBody(
                Return(CallFreeFn::TestStaticVarInFunction(Literal<bool>(false)))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        TestStaticVarInFunction(true /*reset*/);
        int expected = 123;
        for (int i = 0; i < 10; i++)
        {
            expected += 12;
            ReleaseAssert(TestStaticVarInFunction(false /*reset*/) == expected);
            expected += 12;
            ReleaseAssert(interpFn() == expected);
        }
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        TestStaticVarInFunction(true /*reset*/);
        int expected = 123;
        for (int i = 0; i < 10; i++)
        {
            expected += 12;
            ReleaseAssert(TestStaticVarInFunction(false /*reset*/) == expected);
            expected += 12;
            ReleaseAssert(interpFn() == expected);
        }
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        TestStaticVarInFunction(true /*reset*/);
        int expected = 123;
        for (int i = 0; i < 10; i++)
        {
            expected += 12;
            ReleaseAssert(TestStaticVarInFunction(false /*reset*/) == expected);
            expected += 12;
            ReleaseAssert(jitFn() == expected);
        }
    }
}

TEST(SanityCallCppFn, ConstantWithSignificantAddress)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = TestConstantClass*(*)();
    {
        auto [fn] = NewFunction<FnPrototype>("testfn");
        fn.SetBody(
                Return(CallFreeFn::TestConstantWithSignificantAddress())
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        const TestConstantClass* expectedAddr = TestConstantWithSignificantAddress();
        const TestConstantClass* actualAddr = interpFn();
        ReleaseAssert(expectedAddr == actualAddr);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        const TestConstantClass* expectedAddr = TestConstantWithSignificantAddress();
        const TestConstantClass* actualAddr = interpFn();
        ReleaseAssert(expectedAddr == actualAddr);
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        const TestConstantClass* expectedAddr = TestConstantWithSignificantAddress();
        const TestConstantClass* actualAddr = jitFn();
        ReleaseAssert(expectedAddr == actualAddr);
    }
}

TEST(SanityCallCppFn, ConstantWithInsignificantAddress)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = bool(*)(uint8_t*);
    {
        auto [fn, x] = NewFunction<FnPrototype>("testfn");
        fn.SetBody(
                Return(CallFreeFn::TestConstantWithInsignificantAddress(x))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        ReleaseAssert(interpFn(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("123"))) == false);
        ReleaseAssert(interpFn(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("12345678"))) == true);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        ReleaseAssert(interpFn(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("123"))) == false);
        ReleaseAssert(interpFn(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("12345678"))) == true);
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        ReleaseAssert(jitFn(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("123"))) == false);
        ReleaseAssert(jitFn(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("12345678"))) == true);
    }
}

TEST(SanityCallCppFn, StringInternQuirkyBehavior)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = uint8_t*(*)();
    {
        auto [fn] = NewFunction<FnPrototype>("testfn");
        fn.SetBody(
                Return(CallFreeFn::StringInterningQuirkyBehavior())
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        const uint8_t* v1 = StringInterningQuirkyBehavior();
        const uint8_t* v2 = interpFn();
        ReleaseAssert(v1 == v2);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        const uint8_t* v1 = StringInterningQuirkyBehavior();
        const uint8_t* v2 = interpFn();
        ReleaseAssert(v1 == v2);
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        const uint8_t* v1 = StringInterningQuirkyBehavior();
        const uint8_t* v2 = jitFn();
        if (!x_isDebugBuild)
        {
            // in release build, the function should have been optimized out,
            // resulting in two different symbols
            //
            ReleaseAssert(v1 != v2);
            ReleaseAssert(strcmp(reinterpret_cast<const char*>(v1), reinterpret_cast<const char*>(v2)) == 0);
        }
        else
        {
            // in debug build, no inlining happens, we should still be calling the host process's function
            //
            ReleaseAssert(v1 == v2);
        }
    }
}

TEST(SanityCallCppFn, UnexpectedException_Interp)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(int);
    {
        auto [fn, r] = NewFunction<FnPrototype>("testfn");
        fn.SetBody(
                CallFreeFn::TestNoExceptButThrows(r)
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        interpFn(345);  // nothing should happen

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wused-but-marked-unused"
#pragma clang diagnostic ignored "-Wcovered-switch-default"

        fprintf(stdout, "DeathTest: Expecting program termination...\n");
        fflush(stdout);
        ASSERT_DEATH(interpFn(123), "");

#pragma clang diagnostic pop
    }

    thread_pochiVMContext->m_curModule->PrepareForFastInterp();
    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        interpFn(345);  // nothing should happen

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wused-but-marked-unused"
#pragma clang diagnostic ignored "-Wcovered-switch-default"

        fprintf(stdout, "DeathTest: Expecting program termination...\n");
        fflush(stdout);
        ASSERT_DEATH(interpFn(123), "");

#pragma clang diagnostic pop
    }
}

TEST(SanityCallCppFn, UnexpectedException_LLVM)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(int);
    {
        auto [fn, r] = NewFunction<FnPrototype>("testfn");
        fn.SetBody(
                CallFreeFn::TestNoExceptButThrows(r)
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        jitFn(345);  // nothing should happen

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wused-but-marked-unused"
#pragma clang diagnostic ignored "-Wcovered-switch-default"

        fprintf(stdout, "DeathTest: Expecting program termination...\n");
        fflush(stdout);
        ASSERT_DEATH(jitFn(123), "");

#pragma clang diagnostic pop
    }
}

TEST(SanityCallCppFn, Exception_PropagateThrough_1)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(CtorDtorOrderRecorder*);
    {
        auto [fn, r] = NewFunction<FnPrototype>("testfn");
        auto v1 = fn.NewVariable<TestDestructor2>();
        auto v2 = fn.NewVariable<TestDestructor2>();
        auto v3 = fn.NewVariable<TestDestructor2>();
        auto v4 = fn.NewVariable<TestDestructor2>();
        auto v5 = fn.NewVariable<TestDestructor2>();
        auto v6 = fn.NewVariable<TestDestructor2>();
        auto v7 = fn.NewVariable<TestDestructor2>();
        auto v8 = fn.NewVariable<TestDestructor2>();
        auto v9 = fn.NewVariable<TestDestructor2>();
        auto v10 = fn.NewVariable<TestDestructor2>();
        auto v11 = fn.NewVariable<TestDestructor2>();
        auto v12 = fn.NewVariable<TestDestructor2>();
        auto v13 = fn.NewVariable<TestDestructor2>();
        auto v14 = fn.NewVariable<TestDestructor2>();
        fn.SetBody(
                r->PushMaybeThrow(Literal<int>(1000)),
                Declare(v1, Constructor<TestDestructor2>(r, Literal<int>(1))),
                Declare(v2, Constructor<TestDestructor2>(r, Literal<int>(2))),
                r->PushMaybeThrow(Literal<int>(1001)),
                Declare(v3, Constructor<TestDestructor2>(r, Literal<int>(3))),
                Scope(
                    r->PushMaybeThrow(Literal<int>(1002)),
                    Declare(v4, Constructor<TestDestructor2>(r, Literal<int>(4))),
                    r->PushMaybeThrow(Literal<int>(1003)),
                    Declare(v5, Constructor<TestDestructor2>(r, Literal<int>(5))),
                    Declare(v6, Constructor<TestDestructor2>(r, Literal<int>(6))),
                    r->PushMaybeThrow(Literal<int>(1004))
                ),
                r->PushMaybeThrow(Literal<int>(1005)),
                Declare(v7, Constructor<TestDestructor2>(r, Literal<int>(7))),
                Scope(
                    r->PushMaybeThrow(Literal<int>(1006)),
                    Declare(v8, Constructor<TestDestructor2>(r, Literal<int>(8))),
                    Declare(v9, Constructor<TestDestructor2>(r, Literal<int>(9))),
                    Scope(
                        r->PushMaybeThrow(Literal<int>(1007)),
                        Declare(v10, Constructor<TestDestructor2>(r, Literal<int>(10))),
                        Scope(
                            r->PushMaybeThrow(Literal<int>(1008)),
                            Declare(v11, Constructor<TestDestructor2>(r, Literal<int>(11))),
                            r->PushMaybeThrow(Literal<int>(1009))
                        ),
                        Declare(v12, Constructor<TestDestructor2>(r, Literal<int>(12))),
                        r->PushMaybeThrow(Literal<int>(1010))
                    ),
                    r->PushMaybeThrow(Literal<int>(1011)),
                    Declare(v13, Constructor<TestDestructor2>(r, Literal<int>(13))),
                    r->PushMaybeThrow(Literal<int>(1012)),
                    r->PushMaybeThrow(Literal<int>(1013))
                ),
                r->PushMaybeThrow(Literal<int>(1014)),
                Declare(v14, Constructor<TestDestructor2>(r, Literal<int>(14))),
                r->PushMaybeThrow(Literal<int>(1015))
        );
    }

    auto testFn = [](std::function<std::remove_pointer_t<FnPrototype>> f, int value, const std::vector<int>& expectedAns, bool expectThrow) {
        CtorDtorOrderRecorder r;
        r.throwValue = value;
        try {
            f(&r);
            ReleaseAssert(!expectThrow);
        } catch(int v) {
            if (expectThrow) {
                ReleaseAssert(v == value);
            } else {
                ReleaseAssert(false);
            }
        } catch(...) {
            ReleaseAssert(false);
        }
        ReleaseAssert(r.order == expectedAns);
    };

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();

    std::vector<int> expectedAns1000 {
            1000
    };
    std::vector<int> expectedAns1 {
            1000, 1
    };
    std::vector<int> expectedAns2 {
            1000, 1, 2, -1
    };
    std::vector<int> expectedAns1001 {
            1000, 1, 2, 1001, -2, -1
    };
    std::vector<int> expectedAns3 {
            1000, 1, 2, 1001, 3, -2, -1
    };
    std::vector<int> expectedAns1002 {
            1000, 1, 2, 1001, 3, 1002, -3, -2, -1
    };
    std::vector<int> expectedAns4 {
            1000, 1, 2, 1001, 3, 1002, 4, -3, -2, -1
    };
    std::vector<int> expectedAns1003 {
            1000, 1, 2, 1001, 3, 1002, 4, 1003, -4, -3, -2, -1
    };
    std::vector<int> expectedAns5 {
            1000, 1, 2, 1001, 3, 1002, 4, 1003, 5, -4, -3, -2, -1
    };
    std::vector<int> expectedAns6 {
            1000, 1, 2, 1001, 3, 1002, 4, 1003, 5, 6, -5, -4, -3, -2, -1
    };
    std::vector<int> expectedAns1004 {
            1000, 1, 2, 1001, 3, 1002, 4, 1003, 5, 6, 1004, -6, -5, -4, -3, -2, -1
    };
    std::vector<int> expectedAns1005 {
            1000, 1, 2, 1001, 3, 1002, 4, 1003, 5, 6, 1004, -6, -5, -4, 1005, -3, -2, -1
    };
    std::vector<int> expectedAns7 {
            1000, 1, 2, 1001, 3, 1002, 4, 1003, 5, 6, 1004, -6, -5, -4, 1005, 7, -3, -2, -1
    };
    std::vector<int> expectedAns1006 {
            1000, 1, 2, 1001, 3, 1002, 4, 1003, 5, 6, 1004, -6, -5, -4, 1005, 7, 1006, -7, -3, -2, -1
    };
    std::vector<int> expectedAns8 {
            1000, 1, 2, 1001, 3, 1002, 4, 1003, 5, 6, 1004, -6, -5, -4, 1005, 7, 1006, 8, -7, -3, -2, -1
    };
    std::vector<int> expectedAns9 {
            1000, 1, 2, 1001, 3, 1002, 4, 1003, 5, 6, 1004, -6, -5, -4, 1005, 7, 1006, 8, 9, -8, -7, -3, -2, -1
    };
    std::vector<int> expectedAns1007 {
            1000, 1, 2, 1001, 3, 1002, 4, 1003, 5, 6, 1004, -6, -5, -4, 1005, 7, 1006, 8, 9, 1007,
            -9, -8, -7, -3, -2, -1
    };
    std::vector<int> expectedAns10 {
            1000, 1, 2, 1001, 3, 1002, 4, 1003, 5, 6, 1004, -6, -5, -4, 1005, 7, 1006, 8, 9, 1007,
            10, -9, -8, -7, -3, -2, -1
    };
    std::vector<int> expectedAns1008 {
            1000, 1, 2, 1001, 3, 1002, 4, 1003, 5, 6, 1004, -6, -5, -4, 1005, 7, 1006, 8, 9, 1007,
            10, 1008, -10, -9, -8, -7, -3, -2, -1
    };
    std::vector<int> expectedAns11 {
            1000, 1, 2, 1001, 3, 1002, 4, 1003, 5, 6, 1004, -6, -5, -4, 1005, 7, 1006, 8, 9, 1007,
            10, 1008, 11, -10, -9, -8, -7, -3, -2, -1
    };
    std::vector<int> expectedAns1009 {
            1000, 1, 2, 1001, 3, 1002, 4, 1003, 5, 6, 1004, -6, -5, -4, 1005, 7, 1006, 8, 9, 1007,
            10, 1008, 11, 1009, -11, -10, -9, -8, -7, -3, -2, -1
    };
    std::vector<int> expectedAns12 {
            1000, 1, 2, 1001, 3, 1002, 4, 1003, 5, 6, 1004, -6, -5, -4, 1005, 7, 1006, 8, 9, 1007,
            10, 1008, 11, 1009, -11, 12, -10, -9, -8, -7, -3, -2, -1
    };
    std::vector<int> expectedAns1010 {
            1000, 1, 2, 1001, 3, 1002, 4, 1003, 5, 6, 1004, -6, -5, -4, 1005, 7, 1006, 8, 9, 1007,
            10, 1008, 11, 1009, -11, 12, 1010, -12, -10, -9, -8, -7, -3, -2, -1
    };
    std::vector<int> expectedAns1011 {
            1000, 1, 2, 1001, 3, 1002, 4, 1003, 5, 6, 1004, -6, -5, -4, 1005, 7, 1006, 8, 9, 1007,
            10, 1008, 11, 1009, -11, 12, 1010, -12, -10, 1011, -9, -8, -7, -3, -2, -1
    };
    std::vector<int> expectedAns13 {
            1000, 1, 2, 1001, 3, 1002, 4, 1003, 5, 6, 1004, -6, -5, -4, 1005, 7, 1006, 8, 9, 1007,
            10, 1008, 11, 1009, -11, 12, 1010, -12, -10, 1011, 13, -9, -8, -7, -3, -2, -1
    };
    std::vector<int> expectedAns1012 {
            1000, 1, 2, 1001, 3, 1002, 4, 1003, 5, 6, 1004, -6, -5, -4, 1005, 7, 1006, 8, 9, 1007,
            10, 1008, 11, 1009, -11, 12, 1010, -12, -10, 1011, 13, 1012, -13, -9, -8, -7, -3, -2, -1
    };
    std::vector<int> expectedAns1013 {
            1000, 1, 2, 1001, 3, 1002, 4, 1003, 5, 6, 1004, -6, -5, -4, 1005, 7, 1006, 8, 9, 1007,
            10, 1008, 11, 1009, -11, 12, 1010, -12, -10, 1011, 13, 1012, 1013, -13, -9, -8, -7, -3, -2, -1
    };
    std::vector<int> expectedAns1014 {
            1000, 1, 2, 1001, 3, 1002, 4, 1003, 5, 6, 1004, -6, -5, -4, 1005, 7, 1006, 8, 9, 1007,
            10, 1008, 11, 1009, -11, 12, 1010, -12, -10, 1011, 13, 1012, 1013, -13, -9, -8, 1014, -7, -3, -2, -1
    };
    std::vector<int> expectedAns14 {
            1000, 1, 2, 1001, 3, 1002, 4, 1003, 5, 6, 1004, -6, -5, -4, 1005, 7, 1006, 8, 9, 1007,
            10, 1008, 11, 1009, -11, 12, 1010, -12, -10, 1011, 13, 1012, 1013, -13, -9, -8, 1014, 14, -7, -3, -2, -1
    };
    std::vector<int> expectedAns1015 {
            1000, 1, 2, 1001, 3, 1002, 4, 1003, 5, 6, 1004, -6, -5, -4, 1005, 7, 1006, 8, 9, 1007,
            10, 1008, 11, 1009, -11, 12, 1010, -12, -10, 1011, 13, 1012, 1013, -13, -9, -8, 1014, 14, 1015,
            -14, -7, -3, -2, -1
    };
    std::vector<int> expectedAns1016 {
            1000, 1, 2, 1001, 3, 1002, 4, 1003, 5, 6, 1004, -6, -5, -4, 1005, 7, 1006, 8, 9, 1007,
            10, 1008, 11, 1009, -11, 12, 1010, -12, -10, 1011, 13, 1012, 1013, -13, -9, -8, 1014, 14, 1015,
            -14, -7, -3, -2, -1
    };

    {
        auto _interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");
        std::function<std::remove_pointer_t<FnPrototype>> interpFn = [_interpFn](CtorDtorOrderRecorder* param) {
            return _interpFn(param);
        };
        testFn(interpFn, 1000, expectedAns1000, true);
        testFn(interpFn, 1001, expectedAns1001, true);
        testFn(interpFn, 1002, expectedAns1002, true);
        testFn(interpFn, 1003, expectedAns1003, true);
        testFn(interpFn, 1004, expectedAns1004, true);
        testFn(interpFn, 1005, expectedAns1005, true);
        testFn(interpFn, 1006, expectedAns1006, true);
        testFn(interpFn, 1007, expectedAns1007, true);
        testFn(interpFn, 1008, expectedAns1008, true);
        testFn(interpFn, 1009, expectedAns1009, true);
        testFn(interpFn, 1010, expectedAns1010, true);
        testFn(interpFn, 1011, expectedAns1011, true);
        testFn(interpFn, 1012, expectedAns1012, true);
        testFn(interpFn, 1013, expectedAns1013, true);
        testFn(interpFn, 1014, expectedAns1014, true);
        testFn(interpFn, 1015, expectedAns1015, true);
        testFn(interpFn, 1016, expectedAns1016, false);
        testFn(interpFn, 1, expectedAns1, true);
        testFn(interpFn, 2, expectedAns2, true);
        testFn(interpFn, 3, expectedAns3, true);
        testFn(interpFn, 4, expectedAns4, true);
        testFn(interpFn, 5, expectedAns5, true);
        testFn(interpFn, 6, expectedAns6, true);
        testFn(interpFn, 7, expectedAns7, true);
        testFn(interpFn, 8, expectedAns8, true);
        testFn(interpFn, 9, expectedAns9, true);
        testFn(interpFn, 10, expectedAns10, true);
        testFn(interpFn, 11, expectedAns11, true);
        testFn(interpFn, 12, expectedAns12, true);
        testFn(interpFn, 13, expectedAns13, true);
        testFn(interpFn, 14, expectedAns14, true);
    }

    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        testFn(interpFn, 1000, expectedAns1000, true);
        testFn(interpFn, 1001, expectedAns1001, true);
        testFn(interpFn, 1002, expectedAns1002, true);
        testFn(interpFn, 1003, expectedAns1003, true);
        testFn(interpFn, 1004, expectedAns1004, true);
        testFn(interpFn, 1005, expectedAns1005, true);
        testFn(interpFn, 1006, expectedAns1006, true);
        testFn(interpFn, 1007, expectedAns1007, true);
        testFn(interpFn, 1008, expectedAns1008, true);
        testFn(interpFn, 1009, expectedAns1009, true);
        testFn(interpFn, 1010, expectedAns1010, true);
        testFn(interpFn, 1011, expectedAns1011, true);
        testFn(interpFn, 1012, expectedAns1012, true);
        testFn(interpFn, 1013, expectedAns1013, true);
        testFn(interpFn, 1014, expectedAns1014, true);
        testFn(interpFn, 1015, expectedAns1015, true);
        testFn(interpFn, 1016, expectedAns1016, false);
        testFn(interpFn, 1, expectedAns1, true);
        testFn(interpFn, 2, expectedAns2, true);
        testFn(interpFn, 3, expectedAns3, true);
        testFn(interpFn, 4, expectedAns4, true);
        testFn(interpFn, 5, expectedAns5, true);
        testFn(interpFn, 6, expectedAns6, true);
        testFn(interpFn, 7, expectedAns7, true);
        testFn(interpFn, 8, expectedAns8, true);
        testFn(interpFn, 9, expectedAns9, true);
        testFn(interpFn, 10, expectedAns10, true);
        testFn(interpFn, 11, expectedAns11, true);
        testFn(interpFn, 12, expectedAns12, true);
        testFn(interpFn, 13, expectedAns13, true);
        testFn(interpFn, 14, expectedAns14, true);
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        testFn(jitFn, 1000, expectedAns1000, true);
        testFn(jitFn, 1001, expectedAns1001, true);
        testFn(jitFn, 1002, expectedAns1002, true);
        testFn(jitFn, 1003, expectedAns1003, true);
        testFn(jitFn, 1004, expectedAns1004, true);
        testFn(jitFn, 1005, expectedAns1005, true);
        testFn(jitFn, 1006, expectedAns1006, true);
        testFn(jitFn, 1007, expectedAns1007, true);
        testFn(jitFn, 1008, expectedAns1008, true);
        testFn(jitFn, 1009, expectedAns1009, true);
        testFn(jitFn, 1010, expectedAns1010, true);
        testFn(jitFn, 1011, expectedAns1011, true);
        testFn(jitFn, 1012, expectedAns1012, true);
        testFn(jitFn, 1013, expectedAns1013, true);
        testFn(jitFn, 1014, expectedAns1014, true);
        testFn(jitFn, 1015, expectedAns1015, true);
        testFn(jitFn, 1016, expectedAns1016, false);
        testFn(jitFn, 1, expectedAns1, true);
        testFn(jitFn, 2, expectedAns2, true);
        testFn(jitFn, 3, expectedAns3, true);
        testFn(jitFn, 4, expectedAns4, true);
        testFn(jitFn, 5, expectedAns5, true);
        testFn(jitFn, 6, expectedAns6, true);
        testFn(jitFn, 7, expectedAns7, true);
        testFn(jitFn, 8, expectedAns8, true);
        testFn(jitFn, 9, expectedAns9, true);
        testFn(jitFn, 10, expectedAns10, true);
        testFn(jitFn, 11, expectedAns11, true);
        testFn(jitFn, 12, expectedAns12, true);
        testFn(jitFn, 13, expectedAns13, true);
        testFn(jitFn, 14, expectedAns14, true);
    }
}

TEST(SanityCallCppFn, MemberObjectAccessor_1)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(std::pair<int, double>*);
    {
        auto [fn, r] = NewFunction<FnPrototype>("testfn");
        fn.SetBody(
                Return(r->first())
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        std::pair<int, double> v = std::make_pair(123, 456.789);
        ReleaseAssert(interpFn(&v) == 123);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        std::pair<int, double> v = std::make_pair(123, 456.789);
        ReleaseAssert(interpFn(&v) == 123);
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        std::pair<int, double> v = std::make_pair(123, 456.789);
        ReleaseAssert(jitFn(&v) == 123);
    }
}

TEST(SanityCallCppFn, MemberObjectAccessor_2)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = void(*)(std::pair<int, double>*, double);
    {
        auto [fn, r, v] = NewFunction<FnPrototype>("testfn");
        fn.SetBody(
                Assign(r->second(), v)
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        std::pair<int, double> v = std::make_pair(123, 456.789);
        interpFn(&v, 543.21);
        ReleaseAssert(fabs(v.second - 543.21) < 1e-12);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        std::pair<int, double> v = std::make_pair(123, 456.789);
        interpFn(&v, 543.21);
        ReleaseAssert(fabs(v.second - 543.21) < 1e-12);
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        std::pair<int, double> v = std::make_pair(123, 456.789);
        jitFn(&v, 543.21);
        ReleaseAssert(fabs(v.second - 543.21) < 1e-12);
    }
}

// For functions which definition resides in different translational units,
// the parameters may have different struct names. For example, struct name 'struct.std::pair.xxx' is given
// for each std::pair in a translational unit where xxx is a label, so the label may differ in different
// translational units for the same type. This test tests that our IR processor correctly
// renames all the struct names in other translational units so they match the name used in
// pochivm_register_runtime.cpp (which is the one we used to generate AstCppTypeLLVMTypeName array).
//
TEST(SanityCallCppFn, LLVMTypeMismatchRenaming)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = double(*)(std::pair<double, float>*, std::pair<double, uint64_t>*);
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("testfn");
        fn.SetBody(
                Return(CallFreeFn::TestMismatchedLLVMTypeName(a) + CallFreeFn::TestMismatchedLLVMTypeName2(b))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        std::pair<double, float> a = std::make_pair(123.45, 456.789);
        std::pair<double, uint64_t> b = std::make_pair(876.54, 321);
        double r = interpFn(&a, &b);
        ReleaseAssert(fabs(r - (123.45 + 456.789 + 876.54 + 321)) < 1e-5);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        std::pair<double, float> a = std::make_pair(123.45, 456.789);
        std::pair<double, uint64_t> b = std::make_pair(876.54, 321);
        double r = interpFn(&a, &b);
        ReleaseAssert(fabs(r - (123.45 + 456.789 + 876.54 + 321)) < 1e-5);
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        std::pair<double, float> a = std::make_pair(123.45, 456.789);
        std::pair<double, uint64_t> b = std::make_pair(876.54, 321);
        double r = jitFn(&a, &b);
        ReleaseAssert(fabs(r - (123.45 + 456.789 + 876.54 + 321)) < 1e-5);
    }
}

TEST(SanityCallCppFn, LLVMTypeMismatchRenaming_2)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype1 = uint64_t(*)(std::pair<uint16_t, uint32_t>*);
    {
        auto [fn, a] = NewFunction<FnPrototype1>("testfn");
        fn.SetBody(
                Return(CallFreeFn::TestMismatchedLLVMTypeName4(a))
        );
    }

    using FnPrototype2 = uint64_t(*)(std::pair<uint32_t, uint16_t>*);
    {
        auto [fn, a] = NewFunction<FnPrototype2>("testfn2");
        fn.SetBody(
                Return(CallFreeFn::TestMismatchedLLVMTypeName3(a))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn1 = thread_pochiVMContext->m_curModule->
                GetDebugInterpGeneratedFunction<FnPrototype1>("testfn");
        auto interpFn2 = thread_pochiVMContext->m_curModule->
                GetDebugInterpGeneratedFunction<FnPrototype2>("testfn2");

        std::pair<uint16_t, uint32_t> v1 = std::make_pair(123, 456);
        ReleaseAssert(interpFn1(&v1) == 123 + 456);
        std::pair<uint32_t, uint16_t> v2 = std::make_pair(789, 654);
        ReleaseAssert(interpFn2(&v2) == 789 + 654);
    }

    {
        FastInterpFunction<FnPrototype1> interpFn1 = thread_pochiVMContext->m_curModule->
                GetFastInterpGeneratedFunction<FnPrototype1>("testfn");
        FastInterpFunction<FnPrototype2> interpFn2 = thread_pochiVMContext->m_curModule->
                GetFastInterpGeneratedFunction<FnPrototype2>("testfn2");

        std::pair<uint16_t, uint32_t> v1 = std::make_pair(123, 456);
        ReleaseAssert(interpFn1(&v1) == 123 + 456);
        std::pair<uint32_t, uint16_t> v2 = std::make_pair(789, 654);
        ReleaseAssert(interpFn2(&v2) == 789 + 654);
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);

        FnPrototype1 jitFn1 = jit.GetFunction<FnPrototype1>("testfn");
        FnPrototype2 jitFn2 = jit.GetFunction<FnPrototype2>("testfn2");

        std::pair<uint16_t, uint32_t> v1 = std::make_pair(123, 456);
        ReleaseAssert(jitFn1(&v1) == 123 + 456);
        std::pair<uint32_t, uint16_t> v2 = std::make_pair(789, 654);
        ReleaseAssert(jitFn2(&v2) == 789 + 654);
    }
}

// Test that 'const PT&' parameter is converted to 'PT'
//
TEST(SanityCallCppFn, ConstRefParameterConversion_1)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype1 = int(*)(int*);
    {
        auto [fn, a] = NewFunction<FnPrototype1>("testfn");
        auto v = fn.NewVariable<int>();
        fn.SetBody(
                Declare(v, 3),
                Return(CallFreeFn::TestConstPrimitiveTypeParam(
                    Literal<int>(1), *a, a, a, v.Addr(), v.Addr()
                ))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn1 = thread_pochiVMContext->m_curModule->
                GetDebugInterpGeneratedFunction<FnPrototype1>("testfn");

        int v = 123;
        ReleaseAssert(interpFn1(&v) == 123 * 3 + 3 * 2 + 1);
    }

    {
        FastInterpFunction<FnPrototype1> interpFn1 = thread_pochiVMContext->m_curModule->
                GetFastInterpGeneratedFunction<FnPrototype1>("testfn");

        int v = 123;
        ReleaseAssert(interpFn1(&v) == 123 * 3 + 3 * 2 + 1);
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);

        FnPrototype1 jitFn1 = jit.GetFunction<FnPrototype1>("testfn");

        int v = 123;
        ReleaseAssert(jitFn1(&v) == 123 * 3 + 3 * 2 + 1);
    }
}

TEST(SanityCallCppFn, ConstRefParameterConversion_2)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype1 = int(*)(int*);
    {
        auto [fn, a] = NewFunction<FnPrototype1>("testfn");
        auto v = fn.NewVariable<int>();
        auto v2 = fn.NewVariable<TestConstPrimitiveTypeCtor>();
        fn.SetBody(
                Declare(v, 3),
                Declare(v2, Constructor<TestConstPrimitiveTypeCtor>(
                    Literal<int>(1), *a, a, a, v.Addr(), v.Addr()
                )),
                Return(v2.value())
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn1 = thread_pochiVMContext->m_curModule->
                GetDebugInterpGeneratedFunction<FnPrototype1>("testfn");

        int v = 123;
        ReleaseAssert(interpFn1(&v) == 123 * 3 + 3 * 2 + 1);
    }

    {
        FastInterpFunction<FnPrototype1> interpFn1 = thread_pochiVMContext->m_curModule->
                GetFastInterpGeneratedFunction<FnPrototype1>("testfn");

        int v = 123;
        ReleaseAssert(interpFn1(&v) == 123 * 3 + 3 * 2 + 1);
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);

        FnPrototype1 jitFn1 = jit.GetFunction<FnPrototype1>("testfn");

        int v = 123;
        ReleaseAssert(jitFn1(&v) == 123 * 3 + 3 * 2 + 1);
    }
}

TEST(SanityCallCppFn, ConstRefParameterConversion_3)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype1 = int*(*)(int*);
    {
        auto [fn, a] = NewFunction<FnPrototype1>("testfn");
        fn.SetBody(
                Return(CallFreeFn::TestAddressOfConstPrimitiveRef(*a))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn1 = thread_pochiVMContext->m_curModule->
                GetDebugInterpGeneratedFunction<FnPrototype1>("testfn");

        int v = 123;
        ReleaseAssert(interpFn1(&v) == &v);
    }

    {
        FastInterpFunction<FnPrototype1> interpFn1 = thread_pochiVMContext->m_curModule->
                GetFastInterpGeneratedFunction<FnPrototype1>("testfn");

        int v = 123;
        ReleaseAssert(interpFn1(&v) == &v);
    }

    thread_pochiVMContext->m_curModule->EmitIR();
    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);

        FnPrototype1 jitFn1 = jit.GetFunction<FnPrototype1>("testfn");

        int v = 123;
        ReleaseAssert(jitFn1(&v) == &v);
    }
}

TEST(SanityCallCppFn, ReturnExprEvaluatedBeforeDestructor)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)();
    {
        auto [fn] = NewFunction<FnPrototype>("testfn");
        auto v = fn.NewVariable<int>();
        auto x = fn.NewVariable<TestDestructor1>();
        fn.SetBody(
                Declare(v, 123),
                Declare(x, Constructor<TestDestructor1>(Literal<int>(456), v.Addr())),
                Return(v)
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        int out = interpFn();
        ReleaseAssert(out == 123);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        int out = interpFn();
        ReleaseAssert(out == 123);
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(x_isDebugBuild);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        int out = jitFn();
        ReleaseAssert(out == 123);
    }
}

TEST(SanityCallCppFn, ExceptionEscapingNoExceptFunction)
{
    // If a C++ exception escaped a function marked as noexcept,
    // std::terminate() shall be called according to C++ standard. This test tests this behavior.
    //

    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int) noexcept;
    {
        auto [fn, v] = NewFunction<FnPrototype>("testfn");
        auto x = fn.NewVariable<TestNonTrivialConstructor>();
        fn.SetBody(
                Declare(x, Variable<TestNonTrivialConstructor>::Create2(v)),
                Return(x.GetValue())
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        int out = interpFn(123);
        ReleaseAssert(out == 123);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wused-but-marked-unused"
#pragma clang diagnostic ignored "-Wcovered-switch-default"

        fprintf(stdout, "DeathTest: Expecting program termination...\n");
        fflush(stdout);
        ASSERT_DEATH(interpFn(12345), "");

#pragma clang diagnostic pop
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        int out = interpFn(123);
        ReleaseAssert(out == 123);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wused-but-marked-unused"
#pragma clang diagnostic ignored "-Wcovered-switch-default"

        fprintf(stdout, "DeathTest: Expecting program termination...\n");
        fflush(stdout);
        ASSERT_DEATH(interpFn(12345), "");

#pragma clang diagnostic pop
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();
        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        int out = jitFn(123);
        ReleaseAssert(out == 123);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wused-but-marked-unused"
#pragma clang diagnostic ignored "-Wcovered-switch-default"

        fprintf(stdout, "DeathTest: Expecting program termination...\n");
        fflush(stdout);
        ASSERT_DEATH(jitFn(12345), "");

#pragma clang diagnostic pop
    }
}

TEST(SanityCallCppFn, ExceptionEscapingNoExceptFunction_2)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int) noexcept;
    {
        auto [fn, v] = NewFunction<FnPrototype>("testfn");
        fn.SetBody(
                If(v == Literal<int>(12345)).Then(
                    Throw(v)
                ),
                Return(v)
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        int out = interpFn(123);
        ReleaseAssert(out == 123);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wused-but-marked-unused"
#pragma clang diagnostic ignored "-Wcovered-switch-default"

        fprintf(stdout, "DeathTest: Expecting program termination...\n");
        fflush(stdout);
        ASSERT_DEATH(interpFn(12345), "");

#pragma clang diagnostic pop
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        int out = interpFn(123);
        ReleaseAssert(out == 123);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wused-but-marked-unused"
#pragma clang diagnostic ignored "-Wcovered-switch-default"

        fprintf(stdout, "DeathTest: Expecting program termination...\n");
        fflush(stdout);
        ASSERT_DEATH(interpFn(12345), "");

#pragma clang diagnostic pop
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();
        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        int out = jitFn(123);
        ReleaseAssert(out == 123);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wused-but-marked-unused"
#pragma clang diagnostic ignored "-Wcovered-switch-default"

        fprintf(stdout, "DeathTest: Expecting program termination...\n");
        fflush(stdout);
        ASSERT_DEATH(jitFn(12345), "");

#pragma clang diagnostic pop
    }
}

TEST(SanityCallCppFn, CharTypeSanity_1)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = char*(*)(std::string*) noexcept;
    {
        auto [fn, v] = NewFunction<FnPrototype>("testfn");
        fn.SetBody(
                Return(v->c_str())
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        std::string s("12345");
        char* out = interpFn(&s);
        ReleaseAssert(out == s.c_str());
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        std::string s("12345");
        char* out = interpFn(&s);
        ReleaseAssert(out == s.c_str());
    }

    thread_pochiVMContext->m_curModule->EmitIR();

    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        if (x_isDebugBuild)
        {
            AssertIsExpectedOutput(dump, "debug_before_opt");
        }
        else
        {
            AssertIsExpectedOutput(dump, "nondebug_before_opt");
        }
    }

    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    if (!x_isDebugBuild)
    {
        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        thread_pochiVMContext->m_curModule->GetBuiltLLVMModule()->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump, "after_opt");
    }

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        std::string s("12345");
        char* out = jitFn(&s);
        ReleaseAssert(out == s.c_str());
    }
}

TEST(SanityCallCppFn, CharTypeSanity_2)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = char(*)(std::string*) noexcept;
    {
        auto [fn, v] = NewFunction<FnPrototype>("testfn");
        fn.SetBody(
                Return((*v)[Literal<size_t>(2)])
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        std::string s("12345");
        char out = interpFn(&s);
        ReleaseAssert(out == s[2]);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        std::string s("12345");
        char out = interpFn(&s);
        ReleaseAssert(out == s[2]);
    }

    thread_pochiVMContext->m_curModule->EmitIR();
    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        std::string s("12345");
        char out = jitFn(&s);
        ReleaseAssert(out == s[2]);
    }
}

TEST(SanityCallCppFn, CharTypeSanity_3)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = char(*)(std::string*) noexcept;
    {
        auto [fn, v] = NewFunction<FnPrototype>("testfn");
        fn.SetBody(
                Return(v->c_str()[2])
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        std::string s("12345");
        char out = interpFn(&s);
        ReleaseAssert(out == s[2]);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        std::string s("12345");
        char out = interpFn(&s);
        ReleaseAssert(out == s[2]);
    }

    thread_pochiVMContext->m_curModule->EmitIR();
    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        std::string s("12345");
        char out = jitFn(&s);
        ReleaseAssert(out == s[2]);
    }
}

TEST(SanityCallCppFn, CharTypeSanity_4)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = size_t(*)(char*) noexcept;
    {
        auto [fn, v] = NewFunction<FnPrototype>("testfn");
        auto s = fn.NewVariable<std::string>();
        fn.SetBody(
                Declare(s, Constructor<std::string>(v)),
                Return(s.size())
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                               GetDebugInterpGeneratedFunction<FnPrototype>("testfn");

        const char* s = "1234567";
        size_t out = interpFn(const_cast<char*>(s));
        ReleaseAssert(out == 7);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");

        const char* s = "1234567";
        size_t out = interpFn(const_cast<char*>(s));
        ReleaseAssert(out == 7);
    }

    thread_pochiVMContext->m_curModule->EmitIR();
    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    {
        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetModule(thread_pochiVMContext->m_curModule);
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("testfn");

        const char* s = "1234567";
        size_t out = jitFn(const_cast<char*>(s));
        ReleaseAssert(out == 7);
    }
}

