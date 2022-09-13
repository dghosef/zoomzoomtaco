#include "gtest/gtest.h"

#include "fastinterp/fastinterp.hpp"
#include "pochivm/pochivm.h"
#include "codegen_context.hpp"
#include "test_util_helper.h"

using namespace PochiVM;

TEST(TestFastInterp, Sanity_1)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int, int) noexcept;
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                Return(a + b)
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int ret = interpFn(123, -456);
        ReleaseAssert(ret == 123 - 456);
    }
}

TEST(TestFastInterp, Sanity_2)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = double(*)(double, double) noexcept;
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                Return(a * b)
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        double ret = interpFn(12.345, 678.9);
        ReleaseAssert(fabs(ret - 12.345 * 678.9) < 1e-10);
    }
}

TEST(TestFastInterp, Sanity_3)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int, int) noexcept;
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                Return((a + b) + ((a + b) + ((a + b) + ((a + b) + ((a + b) + (a + b))))))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int ret = interpFn(123, -456);
        ReleaseAssert(ret == 6 * (123 - 456));
    }
}

TEST(TestFastInterp, Sanity_4)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int, int) noexcept;
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                Return((((((a + b) + (a + b)) + (a + b)) + (a + b)) + (a + b)) + (a + b))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int ret = interpFn(123, -456);
        ReleaseAssert(ret == 6 * (123 - 456));
    }
}

TEST(TestFastInterp, Sanity_5)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int, int) noexcept;
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                Return((((((a + b) + (a + b)) + ((a + b) + (a + b))) +
                       (((a + b) + (a + b)) + ((a + b) + (a + b)))) +
                       ((((a + b) + (a + b)) + ((a + b) + (a + b))) +
                       (((a + b) + (a + b)) + ((a + b) + (a + b))))) +
                       (((((a + b) + (a + b)) + ((a + b) + (a + b))) +
                       (((a + b) + (a + b)) + ((a + b) + (a + b)))) +
                       ((((a + b) + (a + b)) + ((a + b) + (a + b))) +
                       (((a + b) + (a + b)) + ((a + b) + (a + b))))))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int ret = interpFn(123, -456);
        ReleaseAssert(ret == 32 * (123 - 456));
    }
}

TEST(TestFastInterp, Sanity_6)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int, int) noexcept;
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                Return(((a + b) + a) + (b + (a + b)) + ((a + b) + Literal<int>(2)) + (Literal<int>(1) + (a + b)))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int ret = interpFn(123, -456);
        ReleaseAssert(ret == 5 * (123 - 456) + 3);
    }
}

TEST(TestFastInterp, Sanity_7)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int*, int*) noexcept;
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                Return(*a + *b)
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int x = 123;
        int y = -456;
        int ret = interpFn(&x, &y);
        ReleaseAssert(ret == 123 - 456);
    }
}

TEST(TestFastInterp, Sanity_8)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int*, int*) noexcept;
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                Return((*a + (*b + Literal<int>(1))) + ((Literal<int>(2) + *b) + *a))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int x = 123;
        int y = -456;
        int ret = interpFn(&x, &y);
        ReleaseAssert(ret == 2 * (123 - 456) + 3);
    }
}

TEST(TestFastInterp, Sanity_9)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int**, int**) noexcept;
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                Return((**a + (**b + Literal<int>(1))) + ((Literal<int>(2) + **b) + **a))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int x = 123;
        int y = -456;
        int* xx = &x;
        int* yy = &y;
        int ret = interpFn(&xx, &yy);
        ReleaseAssert(ret == 2 * (123 - 456) + 3);
    }
}

TEST(TestFastInterp, Sanity_10)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int***, int***) noexcept;
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                Return((***a + (***b + Literal<int>(1))) + ((Literal<int>(2) + ***b) + ***a))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int x = 123;
        int y = -456;
        int* xx = &x;
        int* yy = &y;
        int** xxx = &xx;
        int** yyy = &yy;
        int ret = interpFn(&xxx, &yyy);
        ReleaseAssert(ret == 2 * (123 - 456) + 3);
    }
}

TEST(TestFastInterp, Sanity_11)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int****, int****) noexcept;
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                Return((****a + (****b + Literal<int>(1))) + ((Literal<int>(2) + ****b) + ****a))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                               GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int x = 123;
        int y = -456;
        int* xx = &x;
        int* yy = &y;
        int** xxx = &xx;
        int** yyy = &yy;
        int*** xxxx = &xxx;
        int*** yyyy = &yyy;
        int ret = interpFn(&xxxx, &yyyy);
        ReleaseAssert(ret == 2 * (123 - 456) + 3);
    }
}

TEST(TestFastInterp, Sanity_12)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int, int) noexcept;
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("callee");

        fn.SetBody(
                Return(a + b)
        );
    }
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                Return(a + Call<FnPrototype>("callee", a + Literal<int>(1), Literal<int>(2) + (a + b)))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                                GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int x = 123;
        int y = -456;
        int ret = interpFn(x, y);
        ReleaseAssert(ret == 3 * 123 - 456 + 3);
    }
}

TEST(TestFastInterp, Sanity_13)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int, int) noexcept;
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("callee");

        fn.SetBody(
                Return(a + b)
        );
    }
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                Return(Call<FnPrototype>("callee", a + Literal<int>(1), Literal<int>(2) + (a + b)) +
                       Call<FnPrototype>("callee", b + Literal<int>(2), Literal<int>(3) + (b + a)))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                                GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int x = 123;
        int y = -456;
        int ret = interpFn(x, y);
        ReleaseAssert(ret == 3 * (123 - 456) + 8);
    }
}

TEST(TestFastInterp, Sanity_14)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int, int) noexcept;
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("callee");

        fn.SetBody(
                Return(a + b)
        );
    }
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                Return(Call<FnPrototype>("callee", a + Literal<int>(1), Literal<int>(2) + (a + b)) +
                       Call<FnPrototype>("callee", b + Literal<int>(2), Literal<int>(3) + (b + a)) +
                       Call<FnPrototype>("callee", a + b, a + b + a + b))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                                GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int x = 123;
        int y = -456;
        int ret = interpFn(x, y);
        ReleaseAssert(ret == 6 * (123 - 456) + 8);
    }
}

TEST(TestFastInterp, Sanity_15)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int, int) noexcept;
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("callee");

        fn.SetBody(
                Return(a + b)
        );
    }
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                Return(Call<FnPrototype>("callee",
                       Call<FnPrototype>("callee", a + Literal<int>(1), Literal<int>(2) + (a + b)),
                       Call<FnPrototype>("callee", b + Literal<int>(2), Literal<int>(3) + (b + a))))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                                GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int x = 123;
        int y = -456;
        int ret = interpFn(x, y);
        ReleaseAssert(ret == 3 * (123 - 456) + 8);
    }
}

TEST(TestFastInterp, Sanity_16)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype1 = int(*)(int, int, int) noexcept;
    {
        auto [fn, a, b, c] = NewFunction<FnPrototype1>("callee");

        fn.SetBody(
                Return(a + b + c)
        );
    }
    using FnPrototype = int(*)(int, int) noexcept;
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                Return(Call<FnPrototype1>("callee",
                       a + b,
                       Call<FnPrototype1>("callee", a + Literal<int>(1), Literal<int>(2) + (a + b),
                                          Call<FnPrototype1>("callee", a, b, Literal<int>(4))),
                       Call<FnPrototype1>("callee", b + Literal<int>(2), Literal<int>(3) + (b + a), a + b + b + a)))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                                GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int x = 123;
        int y = -456;
        int ret = interpFn(x, y);
        ReleaseAssert(ret == 7 * (123 - 456) + 12);
    }
}

TEST(TestFastInterp, Sanity_17)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype1 = int(*)(int, int, int) noexcept;
    {
        auto [fn, a, b, c] = NewFunction<FnPrototype1>("callee");

        fn.SetBody(
                Return(a + b + c)
        );
    }
    using FnPrototype = int(*)(int, int) noexcept;
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                Return(Call<FnPrototype1>("callee",
                       a + b,
                       (((((a + b) + (a + b)) + ((a + b) + (a + b))) +
                       (((a + b) + (a + b)) + ((a + b) + (a + b)))) +
                       ((((a + b) + (a + b)) + ((a + b) + (a + b))) +
                       (((a + b) + (a + b)) + ((a + b) + (a + b))))) +
                       (((((a + b) + (a + b)) + ((a + b) + (a + b))) +
                       (((a + b) + (a + b)) + ((a + b) + (a + b)))) +
                       ((((a + b) + (a + b)) + ((a + b) + (a + b))) +
                       (((a + b) + (a + b)) + ((a + b) + (a + b))))),
                       a + b + a + b))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                                GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int x = 123;
        int y = -456;
        int ret = interpFn(x, y);
        ReleaseAssert(ret == 35 * (123 - 456));
    }
}

TEST(TestFastInterp, Sanity_18)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype1 = int(*)(int, int, int) noexcept;
    {
        auto [fn, a, b, c] = NewFunction<FnPrototype1>("callee");

        fn.SetBody(
                Return(a + b + c)
        );
    }
    using FnPrototype = int(*)(int, int) noexcept;
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                Return(Call<FnPrototype1>("callee",
                       a + b,
                       a + b + a + b,
                       (((((a + b) + (a + b)) + ((a + b) + (a + b))) +
                       (((a + b) + (a + b)) + ((a + b) + (a + b)))) +
                       ((((a + b) + (a + b)) + ((a + b) + (a + b))) +
                       (((a + b) + (a + b)) + ((a + b) + (a + b))))) +
                       (((((a + b) + (a + b)) + ((a + b) + (a + b))) +
                       (((a + b) + (a + b)) + ((a + b) + (a + b)))) +
                       ((((a + b) + (a + b)) + ((a + b) + (a + b))) +
                       (((a + b) + (a + b)) + ((a + b) + (a + b)))))))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                                GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int x = 123;
        int y = -456;
        int ret = interpFn(x, y);
        ReleaseAssert(ret == 35 * (123 - 456));
    }
}

TEST(TestFastInterp, Sanity_19)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype1 = int(*)() noexcept;
    {
        auto [fn] = NewFunction<FnPrototype1>("callee");

        fn.SetBody(
                Return(Literal<int>(-12345))
        );
    }
    using FnPrototype = int(*)(int) noexcept;
    {
        auto [fn, a] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                Return(a + Call<FnPrototype1>("callee") + Call<FnPrototype1>("callee"))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                                GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int x = 123;
        int ret = interpFn(x);
        ReleaseAssert(ret == 2 * -12345 + 123);
    }
}

TEST(TestFastInterp, Sanity_20)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int, int) noexcept;
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("testfn");

        fn.SetBody();
        for (int i = 0; i < 10000; i++)
        {
            fn.GetBody().Append(Assign(a, a + b));
        }
        fn.GetBody().Append(Return(a));
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                                GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int x = 123;
        int y = 45;
        int ret = interpFn(x, y);
        ReleaseAssert(ret == x + y * 10000);
    }
}

TEST(TestFastInterp, Sanity_21)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int) noexcept;
    {
        auto [fn, n] = NewFunction<FnPrototype>("testfn");
        auto i = fn.NewVariable<int>();
        auto sum = fn.NewVariable<int>();
        fn.SetBody(
                Declare(i, 0),
                Declare(sum, 0),
                While(i < n).Do(
                    Assign(sum, sum + i),
                    Assign(i, i + Literal<int>(1))
                ),
                Return(sum)
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                                GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int ret = interpFn(100);
        ReleaseAssert(ret == 100 * 99 / 2);
    }
}

TEST(TestFastInterp, Sanity_22)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int) noexcept;
    {
        auto [fn, n] = NewFunction<FnPrototype>("testfn");
        auto i = fn.NewVariable<int>();
        auto sum = fn.NewVariable<int>();
        fn.SetBody(
                Declare(sum, 0),
                For(Declare(i, 0), i < n, Assign(i, i + Literal<int>(1))).Do(
                    Assign(sum, sum + i)
                ),
                Return(sum)
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                                GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int ret = interpFn(100);
        ReleaseAssert(ret == 100 * 99 / 2);
    }
}

TEST(TestFastInterp, Sanity_23)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int, int) noexcept;
    {
        auto [fn, n, m] = NewFunction<FnPrototype>("testfn");
        auto i = fn.NewVariable<int>();
        auto sum = fn.NewVariable<int>();
        fn.SetBody(
                Declare(i, 0),
                Declare(sum, 0),
                While(i < n).Do(
                    If(i == m).Then(Break()),
                    Assign(sum, sum + i),
                    Assign(i, i + Literal<int>(1))
                ),
                Return(sum)
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                                GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int ret = interpFn(100, 200);
        ReleaseAssert(ret == 100 * 99 / 2);
        ret = interpFn(100, 50);
        ReleaseAssert(ret == 50 * 49 / 2);
    }
}

TEST(TestFastInterp, Sanity_24)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int, int) noexcept;
    {
        auto [fn, n, m] = NewFunction<FnPrototype>("testfn");
        auto i = fn.NewVariable<int>();
        auto sum = fn.NewVariable<int>();
        fn.SetBody(
                Declare(sum, 0),
                For(Declare(i, 0), i < n, Assign(i, i + Literal<int>(1))).Do(
                    If(i == m).Then(Break()),
                    Assign(sum, sum + i)
                ),
                Return(sum)
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                                GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int ret = interpFn(100, 200);
        ReleaseAssert(ret == 100 * 99 / 2);
        ret = interpFn(100, 50);
        ReleaseAssert(ret == 50 * 49 / 2);
    }
}

TEST(TestFastInterp, Sanity_25)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int, int) noexcept;
    {
        auto [fn, n, m] = NewFunction<FnPrototype>("testfn");
        auto i = fn.NewVariable<int>();
        auto sum = fn.NewVariable<int>();
        fn.SetBody(
                Declare(sum, 0),
                For(Declare(i, 0), i < n, Assign(i, i + Literal<int>(1))).Do(
                    Assign(sum, sum + i),
                    If(i >= m).Then(Continue()),
                    Assign(sum, sum + i)
                ),
                Return(sum)
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                                GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int ret = interpFn(100, 200);
        ReleaseAssert(ret == 100 * 99);
        ret = interpFn(100, 50);
        ReleaseAssert(ret == 100 * 99 / 2 + 50 * 49 / 2);
    }
}

TEST(TestFastInterp, Sanity_26)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype1 = int(*)() noexcept;
    {
        auto [fn] = NewFunction<FnPrototype1>("callee");

        fn.SetBody(
                Return(Literal<int>(123))
        );
    }
    using FnPrototype = int(*)(int, int) noexcept;
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                Return((a + b) + (Call<FnPrototype1>("callee") + (a * b)))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                                GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int x = 12;
        int y = -34;
        int ret = interpFn(x, y);
        ReleaseAssert(ret == 12 - 34 + 123 + 12 * -34);
    }
}

TEST(TestFastInterp, Sanity_27)
{
    AutoThreadPochiVMContext apv;
    AutoThreadErrorContext arc;
    AutoThreadLLVMCodegenContext alc;

    thread_pochiVMContext->m_curModule = new AstModule("test");

    using FnPrototype = int(*)(int, int) noexcept;
    {
        auto [fn, a, b] = NewFunction<FnPrototype>("testfn");

        fn.SetBody(
                Return((a + b) + ((((((a + b) + (a + b)) + ((a + b) + (a + b))) +
                (((a + b) + (a + b)) + ((a + b) + (a + b)))) +
                ((((a + b) + (a + b)) + ((a + b) + (a + b))) +
                (((a + b) + (a + b)) + ((a + b) + (a + b))))) +
                (((((a + b) + (a + b)) + ((a + b) + (a + b))) +
                (((a + b) + (a + b)) + ((a + b) + (a + b)))) +
                ((((a + b) + (a + b)) + ((a + b) + (a + b))) +
                (((a + b) + (a + b)) + ((a + b) + (a + b))))) + (a * b)))
        );
    }

    ReleaseAssert(thread_pochiVMContext->m_curModule->Validate());
    thread_pochiVMContext->m_curModule->PrepareForFastInterp();

    {
        FastInterpFunction<FnPrototype> interpFn = thread_pochiVMContext->m_curModule->
                                GetFastInterpGeneratedFunction<FnPrototype>("testfn");
        int x = 12;
        int y = -34;
        int ret = interpFn(x, y);
        ReleaseAssert(ret == 33 * (12 - 34) + 12 * -34);
    }
}
