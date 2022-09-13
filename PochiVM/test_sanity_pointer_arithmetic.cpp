#include "gtest/gtest.h"

#include "pochivm.h"
#include "test_util_helper.h"

using namespace PochiVM;

namespace {

void TestEachInterestingIntParam(const std::function<void(uint64_t, uint64_t)>& testFn)
{
    const static uint64_t choice[4] = {
        static_cast<uint64_t>(123),
        static_cast<uint64_t>(-123),
        static_cast<uint64_t>(0),
        static_cast<uint64_t>(233)
    };

    for (int bitmask = 0; bitmask < (4 << 8); bitmask++)
    {
        uint64_t value = 0;
        int x = bitmask;
        for (int i = 0; i < 8; i++)
        {
            value += (choice[x % 4]) * (static_cast<uint64_t>(1) << (i * 8));
            x /= 4;
        }
        testFn(1000000, value);
    }
}

template<typename B, typename I>
std::function<void(uint64_t, uint64_t)> TestPointerArithmeticInternal(bool isAdd)
{
    static_assert(std::is_pointer<B>::value);
    using FnPrototype = B(*)(B, I);
    auto [fn, b, i] = NewFunction<FnPrototype>("MyFn");

    if (isAdd)
    {
        fn.SetBody(Return(b + i));
    }
    else
    {
        fn.SetBody(Return(b - i));
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();
    thread_pochiVMContext->m_curModule->EmitIR();
    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    // TODO: this leaks. Fix later
    //
    SimpleJIT* jit = new SimpleJIT();

    jit->SetModule(thread_pochiVMContext->m_curModule);

    FnPrototype jitFn = jit->GetFunction<FnPrototype>("MyFn");

    auto interpFn = thread_pochiVMContext->m_curModule->
                           GetDebugInterpGeneratedFunction<FnPrototype>("MyFn");
    ReleaseAssert(interpFn);

    FastInterpFunction<FnPrototype> fastInterpFn = thread_pochiVMContext->m_curModule->
                           GetFastInterpGeneratedFunction<FnPrototype>("MyFn");

    return [isAdd, jitFn, interpFn, fastInterpFn](uint64_t v1, uint64_t v2) -> void {
        B b = reinterpret_cast<B>(v1);
        I i = static_cast<I>(v2);
        B expected;
        if (isAdd)
        {
            expected = b + i;
        }
        else
        {
            expected = b - i;
        }

        B out1 = interpFn(b, i);
        B out2 = jitFn(b, i);
        B out3 = fastInterpFn(b, i);
        ReleaseAssert(out1 == expected);
        ReleaseAssert(out2 == expected);
        ReleaseAssert(out3 == expected);
    };
}

template<typename B, typename I>
void TestPointerArithmetic(bool isAdd)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    std::function<void(uint64_t, uint64_t)> fn = TestPointerArithmeticInternal<B, I>(isAdd);
    TestEachInterestingIntParam(fn);
}

}   // anonymous namespace

TEST(SanityPointerArithmetic, Sanity_1)
{
    for (bool isAdd : {false, true})
    {
#define F(type) TestPointerArithmetic<int*, type>(isAdd);
        FOR_EACH_PRIMITIVE_INT_TYPE
#undef F

#define F(type) TestPointerArithmetic<TestClassA*, type>(isAdd);
        FOR_EACH_PRIMITIVE_INT_TYPE
#undef F
    }
}

TEST(SanityPointerArithmetic, Sanity_2)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(TestClassA*, int);
    auto [fn, b, i] = NewFunction<FnPrototype>("MyFn");

    fn.SetBody(Return((b + i)->GetY()));

    TestClassA tmp[3];
    tmp[0].SetY(123);
    tmp[1].SetY(456);
    tmp[2].SetY(789);

    TestClassA* ptr = tmp + 1;

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    ReleaseAssert(!thread_errorContext->HasError());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();
    thread_pochiVMContext->m_curModule->EmitIR();

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                GetDebugInterpGeneratedFunction<FnPrototype>("MyFn");

        ReleaseAssert(interpFn(ptr, -1) == 123);
        ReleaseAssert(interpFn(ptr, 0) == 456);
        ReleaseAssert(interpFn(ptr, 1) == 789);

    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                GetFastInterpGeneratedFunction<FnPrototype>("MyFn");

        ReleaseAssert(interpFn(ptr, -1) == 123);
        ReleaseAssert(interpFn(ptr, 0) == 456);
        ReleaseAssert(interpFn(ptr, 1) == 789);
    }

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
        AssertIsExpectedOutput(dump, "nondebug_after_opt");
    }

    SimpleJIT jit;
    jit.SetAllowResolveSymbolInHostProcess(true);
    jit.SetModule(thread_pochiVMContext->m_curModule);

    {
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("MyFn");
        ReleaseAssert(jitFn(ptr, -1) == 123);
        ReleaseAssert(jitFn(ptr, 0) == 456);
        ReleaseAssert(jitFn(ptr, 1) == 789);
    }
}

TEST(SanityPointerArithmetic, Sanity_3)
{
    AutoThreadPochiVMContext apv;AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int*, int);
    auto [fn, b, i] = NewFunction<FnPrototype>("MyFn");

    fn.SetBody(Return(*(b + i)));

    int tmp[3] = {123, 456, 789};
    int* ptr = tmp + 1;

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    ReleaseAssert(!thread_errorContext->HasError());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();
    thread_pochiVMContext->m_curModule->EmitIR();
    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                GetDebugInterpGeneratedFunction<FnPrototype>("MyFn");

        ReleaseAssert(interpFn(ptr, -1) == 123);
        ReleaseAssert(interpFn(ptr, 0) == 456);
        ReleaseAssert(interpFn(ptr, 1) == 789);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                GetFastInterpGeneratedFunction<FnPrototype>("MyFn");

        ReleaseAssert(interpFn(ptr, -1) == 123);
        ReleaseAssert(interpFn(ptr, 0) == 456);
        ReleaseAssert(interpFn(ptr, 1) == 789);
    }

    SimpleJIT jit;
    jit.SetAllowResolveSymbolInHostProcess(true);
    jit.SetModule(thread_pochiVMContext->m_curModule);

    {
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("MyFn");
        ReleaseAssert(jitFn(ptr, -1) == 123);
        ReleaseAssert(jitFn(ptr, 0) == 456);
        ReleaseAssert(jitFn(ptr, 1) == 789);
    }
}

namespace {

template<typename LiteralType, LiteralType literalValue, bool isAdd>
void TestLiteralPointerArithmetic()
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int*);
    auto [fn, b] = NewFunction<FnPrototype>("MyFn");

    if (isAdd)
    {
        fn.SetBody(Return(*(b + Literal<LiteralType>(literalValue))));
    }
    else
    {
        fn.SetBody(Return(*(b - Literal<LiteralType>(literalValue))));
    }

    int tmp[3] = {123, 456, 789};

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    ReleaseAssert(!thread_errorContext->HasError());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();
    thread_pochiVMContext->m_curModule->EmitIR();
    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warray-bounds-pointer-arithmetic"

    ssize_t shift = static_cast<ssize_t>(literalValue);
    if (!isAdd)
    {
        shift = -shift;
    }

    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                GetDebugInterpGeneratedFunction<FnPrototype>("MyFn");

        ReleaseAssert(interpFn(tmp - shift) == 123);
        ReleaseAssert(interpFn(tmp - shift + 1) == 456);
        ReleaseAssert(interpFn(tmp - shift + 2) == 789);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                GetFastInterpGeneratedFunction<FnPrototype>("MyFn");

        ReleaseAssert(interpFn(tmp - shift) == 123);
        ReleaseAssert(interpFn(tmp - shift + 1) == 456);
        ReleaseAssert(interpFn(tmp - shift + 2) == 789);
    }

    SimpleJIT jit;
    jit.SetAllowResolveSymbolInHostProcess(true);
    jit.SetModule(thread_pochiVMContext->m_curModule);

    {
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("MyFn");
        ReleaseAssert(jitFn(tmp - shift) == 123);
        ReleaseAssert(jitFn(tmp - shift + 1) == 456);
        ReleaseAssert(jitFn(tmp - shift + 2) == 789);
    }
#pragma clang diagnostic pop
}

}   // anonymous namespace

TEST(SanityPointerArithmetic, Sanity_4)
{
    TestLiteralPointerArithmetic<int8_t, 2, true>();
    TestLiteralPointerArithmetic<int8_t, 1, true>();
    TestLiteralPointerArithmetic<int8_t, 0, true>();
    TestLiteralPointerArithmetic<int8_t, -1, true>();
    TestLiteralPointerArithmetic<int8_t, -2, true>();
    TestLiteralPointerArithmetic<int8_t, 2, false>();
    TestLiteralPointerArithmetic<int8_t, 1, false>();
    TestLiteralPointerArithmetic<int8_t, 0, false>();
    TestLiteralPointerArithmetic<int8_t, -1, false>();
    TestLiteralPointerArithmetic<int8_t, -2, false>();

    TestLiteralPointerArithmetic<int16_t, 2, true>();
    TestLiteralPointerArithmetic<int16_t, 1, true>();
    TestLiteralPointerArithmetic<int16_t, 0, true>();
    TestLiteralPointerArithmetic<int16_t, -1, true>();
    TestLiteralPointerArithmetic<int16_t, -2, true>();
    TestLiteralPointerArithmetic<int16_t, 2, false>();
    TestLiteralPointerArithmetic<int16_t, 1, false>();
    TestLiteralPointerArithmetic<int16_t, 0, false>();
    TestLiteralPointerArithmetic<int16_t, -1, false>();
    TestLiteralPointerArithmetic<int16_t, -2, false>();

    TestLiteralPointerArithmetic<int32_t, 2, true>();
    TestLiteralPointerArithmetic<int32_t, 1, true>();
    TestLiteralPointerArithmetic<int32_t, 0, true>();
    TestLiteralPointerArithmetic<int32_t, -1, true>();
    TestLiteralPointerArithmetic<int32_t, -2, true>();
    TestLiteralPointerArithmetic<int32_t, 2, false>();
    TestLiteralPointerArithmetic<int32_t, 1, false>();
    TestLiteralPointerArithmetic<int32_t, 0, false>();
    TestLiteralPointerArithmetic<int32_t, -1, false>();
    TestLiteralPointerArithmetic<int32_t, -2, false>();

    TestLiteralPointerArithmetic<int64_t, 2, true>();
    TestLiteralPointerArithmetic<int64_t, 1, true>();
    TestLiteralPointerArithmetic<int64_t, 0, true>();
    TestLiteralPointerArithmetic<int64_t, -1, true>();
    TestLiteralPointerArithmetic<int64_t, -2, true>();
    TestLiteralPointerArithmetic<int64_t, 2, false>();
    TestLiteralPointerArithmetic<int64_t, 1, false>();
    TestLiteralPointerArithmetic<int64_t, 0, false>();
    TestLiteralPointerArithmetic<int64_t, -1, false>();
    TestLiteralPointerArithmetic<int64_t, -2, false>();

    TestLiteralPointerArithmetic<uint8_t, 2, true>();
    TestLiteralPointerArithmetic<uint8_t, 1, true>();
    TestLiteralPointerArithmetic<uint8_t, 0, true>();
    TestLiteralPointerArithmetic<uint8_t, 2, false>();
    TestLiteralPointerArithmetic<uint8_t, 1, false>();
    TestLiteralPointerArithmetic<uint8_t, 0, false>();

    TestLiteralPointerArithmetic<uint16_t, 2, true>();
    TestLiteralPointerArithmetic<uint16_t, 1, true>();
    TestLiteralPointerArithmetic<uint16_t, 0, true>();
    TestLiteralPointerArithmetic<uint16_t, 2, false>();
    TestLiteralPointerArithmetic<uint16_t, 1, false>();
    TestLiteralPointerArithmetic<uint16_t, 0, false>();

    TestLiteralPointerArithmetic<uint32_t, 2, true>();
    TestLiteralPointerArithmetic<uint32_t, 1, true>();
    TestLiteralPointerArithmetic<uint32_t, 0, true>();
    TestLiteralPointerArithmetic<uint32_t, 2, false>();
    TestLiteralPointerArithmetic<uint32_t, 1, false>();
    TestLiteralPointerArithmetic<uint32_t, 0, false>();

    TestLiteralPointerArithmetic<uint64_t, 2, true>();
    TestLiteralPointerArithmetic<uint64_t, 1, true>();
    TestLiteralPointerArithmetic<uint64_t, 0, true>();
    TestLiteralPointerArithmetic<uint64_t, static_cast<uint64_t>(-1), true>();
    TestLiteralPointerArithmetic<uint64_t, static_cast<uint64_t>(-2), true>();
    TestLiteralPointerArithmetic<uint64_t, 2, false>();
    TestLiteralPointerArithmetic<uint64_t, 1, false>();
    TestLiteralPointerArithmetic<uint64_t, 0, false>();
    TestLiteralPointerArithmetic<uint64_t, static_cast<uint64_t>(-1), false>();
    TestLiteralPointerArithmetic<uint64_t, static_cast<uint64_t>(-2), false>();

    TestLiteralPointerArithmetic<bool, true, true>();
    TestLiteralPointerArithmetic<bool, false, true>();
    TestLiteralPointerArithmetic<bool, true, false>();
    TestLiteralPointerArithmetic<bool, false, false>();
}

TEST(SanityPointerArithmetic, Sanity_5)
{
    AutoThreadPochiVMContext apv;AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int*);
    auto [fn, b] = NewFunction<FnPrototype>("MyFn");

    fn.SetBody(Return(b[1]));

    int tmp[3] = {123, 456, 789};

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    ReleaseAssert(!thread_errorContext->HasError());
    thread_pochiVMContext->m_curModule->PrepareForDebugInterp();
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();
    thread_pochiVMContext->m_curModule->EmitIR();
    thread_pochiVMContext->m_curModule->OptimizeIRIfNotDebugMode(2 /*optLevel*/);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warray-bounds-pointer-arithmetic"
    {
        auto interpFn = thread_pochiVMContext->m_curModule->
                GetDebugInterpGeneratedFunction<FnPrototype>("MyFn");

        ReleaseAssert(interpFn(tmp - 1) == 123);
        ReleaseAssert(interpFn(tmp) == 456);
        ReleaseAssert(interpFn(tmp + 1) == 789);
    }

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                GetFastInterpGeneratedFunction<FnPrototype>("MyFn");

        ReleaseAssert(interpFn(tmp - 1) == 123);
        ReleaseAssert(interpFn(tmp) == 456);
        ReleaseAssert(interpFn(tmp + 1) == 789);
    }

    SimpleJIT jit;
    jit.SetAllowResolveSymbolInHostProcess(true);
    jit.SetModule(thread_pochiVMContext->m_curModule);

    {
        FnPrototype jitFn = jit.GetFunction<FnPrototype>("MyFn");
        ReleaseAssert(jitFn(tmp - 1) == 123);
        ReleaseAssert(jitFn(tmp) == 456);
        ReleaseAssert(jitFn(tmp + 1) == 789);
    }
#pragma clang diagnostic pop
}
