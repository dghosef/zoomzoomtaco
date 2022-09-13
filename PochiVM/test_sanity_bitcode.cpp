#include "gtest/gtest.h"

#include "pochivm.h"
#include "test_util_helper.h"
#include "codegen_context.hpp"
#include "generated/pochivm_runtime_library_bitcodes.generated.h"
#include "runtime/pochivm_runtime_headers.h"

#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Linker/Linker.h"

using namespace PochiVM;

using namespace llvm;
using namespace llvm::orc;

namespace {

std::unique_ptr<Module> GetModuleFromBitcodeStub(const BitcodeData& bc,
                                                 LLVMContext* context)
{
    SMDiagnostic llvmErr;
    MemoryBufferRef mb(StringRef(reinterpret_cast<const char*>(bc.m_bitcode), bc.m_length),
                       StringRef(bc.m_symbolName));
    std::unique_ptr<Module> module = parseIR(mb, llvmErr, *context);
    ReleaseAssert(module != nullptr);
    return module;
}

std::unique_ptr<ThreadSafeModule> GetThreadSafeModuleFromBitcodeStub(const BitcodeData& bc)
{
    std::unique_ptr<LLVMContext> context(new LLVMContext);
    std::unique_ptr<Module> module = GetModuleFromBitcodeStub(bc, context.get());
    return std::unique_ptr<ThreadSafeModule>(new ThreadSafeModule(std::move(module), std::move(context)));
}

void LinkinBitcodeStub(Module& module /*inout*/,
                       const BitcodeData& bc)
{
    std::unique_ptr<Module> m = GetModuleFromBitcodeStub(bc, &module.getContext());
    Linker linker(module);
    // linkInModule returns true on error
    //
    ReleaseAssert(linker.linkInModule(std::move(m)) == false);
}

}   // anonymous namespace

TEST(SanityBitcode, SanityExecution_1)
{
    const BitcodeData& getY = __pochivm_internal_bc_fb2d7a235b0bee90ed026467;
    const BitcodeData& setY = __pochivm_internal_bc_9e962f83bf07fe71b8a356ec;
    const BitcodeData& getXPlusY = __pochivm_internal_bc_0168482cbf35dd60c2677b26;
    const BitcodeData& getStringY = __pochivm_internal_bc_888a3d0065471b9b797cc710;

    TestClassA a;
    a.m_y = 0;

    SimpleJIT jit;
    std::unique_ptr<ThreadSafeModule> tsm1 = GetThreadSafeModuleFromBitcodeStub(setY);
    std::unique_ptr<ThreadSafeModule> tsm2 = GetThreadSafeModuleFromBitcodeStub(getY);
    std::unique_ptr<ThreadSafeModule> tsm3 = GetThreadSafeModuleFromBitcodeStub(getXPlusY);
    std::unique_ptr<ThreadSafeModule> tsm4 = GetThreadSafeModuleFromBitcodeStub(getStringY);

    using _SetFnPrototype = void(*)(TestClassA*, int);
    using _GetFnPrototype = int(*)(TestClassA*);
    using _GetXPlusYFnPrototype = int(*)(TestClassA*, int);
    using _GetStringYFnPrototype = std::string(*)(TestClassA*);

    jit.SetNonAstModule(std::move(tsm1));
    _SetFnPrototype setFn = jit.GetFunctionNonAst<_SetFnPrototype>(std::string(setY.m_symbolName));

    setFn(&a, 233);
    ReleaseAssert(a.m_y == 233);

    jit.SetNonAstModule(std::move(tsm2));
    _GetFnPrototype getFn = jit.GetFunctionNonAst<_GetFnPrototype>(std::string(getY.m_symbolName));

    {
        int r = getFn(&a);
        ReleaseAssert(r == 233);
        ReleaseAssert(a.m_y == 233);
    }

    jit.SetNonAstModule(std::move(tsm3));
    _GetXPlusYFnPrototype getXPlusYFn = jit.GetFunctionNonAst<_GetXPlusYFnPrototype>(std::string(getXPlusY.m_symbolName));

    {
        int r = getXPlusYFn(&a, 456);
        ReleaseAssert(r == 233 + 456);
        ReleaseAssert(a.m_y == 233);
    }

    jit.SetAllowResolveSymbolInHostProcess(true);
    jit.SetNonAstModule(std::move(tsm4));
    _GetStringYFnPrototype getStringYFn = jit.GetFunctionNonAst<_GetStringYFnPrototype>(std::string(getStringY.m_symbolName));

    {
        std::string r = getStringYFn(&a);
        ReleaseAssert(r == "mystr_233");
    }
}

TEST(SanityBitcode, SanityExecution_2)
{
    const BitcodeData& pushVec = __pochivm_internal_bc_9d40e07b8417ba7543c9b4cd;
    const BitcodeData& sortVector = __pochivm_internal_bc_9211d742c8c7612d69d11c5b;
    const BitcodeData& getVectorSize = __pochivm_internal_bc_ece726fdd4130f1f904091ef;
    const BitcodeData& getVectorSum = __pochivm_internal_bc_2d9dcbc4a90b11f2fd8ca7d6;

    TestClassA a;
    a.m_vec.clear();

    SimpleJIT jit;
    std::unique_ptr<ThreadSafeModule> tsm1 = GetThreadSafeModuleFromBitcodeStub(pushVec);
    std::unique_ptr<ThreadSafeModule> tsm2 = GetThreadSafeModuleFromBitcodeStub(sortVector);
    std::unique_ptr<ThreadSafeModule> tsm3 = GetThreadSafeModuleFromBitcodeStub(getVectorSize);
    std::unique_ptr<ThreadSafeModule> tsm4 = GetThreadSafeModuleFromBitcodeStub(getVectorSum);

    using _PushFnPrototype = void(*)(TestClassA*, int);
    using _SortFnPrototype = int(*)(TestClassA*);
    using _GetSizeFnPrototype = size_t(*)(TestClassA*);
    using _GetSumFnPrototype = int64_t(*)(TestClassA*);

    jit.SetAllowResolveSymbolInHostProcess(true);
    jit.SetNonAstModule(std::move(tsm1));
    _PushFnPrototype pushFn = jit.GetFunctionNonAst<_PushFnPrototype>(std::string(pushVec.m_symbolName));
    // make sure we are getting the pointer to generated code, not host process
    //
    ReleaseAssert(reinterpret_cast<void*>(pushFn) != AstTypeHelper::GetClassMethodPtr(&TestClassA::PushVec));

    for (size_t i = 0; i < 97; i++)
    {
        pushFn(&a, i * 37 % 97);
        ReleaseAssert(a.m_vec.size() == i + 1);
        for (size_t j = 0; j < i; j++)
        {
            ReleaseAssert(a.m_vec[j] == j * 37 % 97);
        }
    }

    jit.SetNonAstModule(std::move(tsm2));
    _SortFnPrototype sortFn = jit.GetFunctionNonAst<_SortFnPrototype>(std::string(sortVector.m_symbolName));

    for (int k = 0; k < 2; k++)
    {
        sortFn(&a);
        ReleaseAssert(a.m_vec.size() == 97);
        for (size_t i = 0; i < 97; i++)
        {
            ReleaseAssert(a.m_vec[i] == static_cast<int>(i));
        }
    }

    jit.SetNonAstModule(std::move(tsm3));
    _GetSizeFnPrototype getSizeFn = jit.GetFunctionNonAst<_GetSizeFnPrototype>(std::string(getVectorSize.m_symbolName));

    for (int k = 0; k < 2; k++)
    {
        ReleaseAssert(a.m_vec.size() == 97);
        size_t r = getSizeFn(&a);
        ReleaseAssert(r == 97);
    }

    jit.SetNonAstModule(std::move(tsm4));
    _GetSumFnPrototype getSumFn = jit.GetFunctionNonAst<_GetSumFnPrototype>(std::string(getVectorSum.m_symbolName));

    for (int k = 0; k < 2; k++)
    {
        ReleaseAssert(a.m_vec.size() == 97);
        int64_t r = getSumFn(&a);
        ReleaseAssert(r == 97 * 96 / 2);
    }
}

TEST(SanityBitcode, SanityExecution_3)
{
    // int FreeFunctionOverloaded<double, double, int>(int, int)
    const BitcodeData& sig1 = __pochivm_internal_bc_bd614a9c9c6a469d1aa5a33c;
    // int FreeFunctionOverloaded<double, double, double>(int, int)
    const BitcodeData& sig2 = __pochivm_internal_bc_a9b7de4eb4a4edee827deb07;
    // int FreeFunctionOverloaded<int, int, int>(int)
    const BitcodeData& sig3 = __pochivm_internal_bc_55fe00ceaaa5ab7945195db3;
    // int FreeFunctionOverloaded<int, int, double>(int)
    const BitcodeData& sig4 = __pochivm_internal_bc_6d29f5afd61bc9b4312e126b;
    // double FreeFunctionOverloaded(double)
    const BitcodeData& sig5 = __pochivm_internal_bc_0c6d6b096143d90aa39723f9;

    TestClassA a;
    a.m_vec.clear();

    SimpleJIT jit;
    std::unique_ptr<ThreadSafeModule> tsm1 = GetThreadSafeModuleFromBitcodeStub(sig1);
    std::unique_ptr<ThreadSafeModule> tsm2 = GetThreadSafeModuleFromBitcodeStub(sig2);
    std::unique_ptr<ThreadSafeModule> tsm3 = GetThreadSafeModuleFromBitcodeStub(sig3);
    std::unique_ptr<ThreadSafeModule> tsm4 = GetThreadSafeModuleFromBitcodeStub(sig4);
    std::unique_ptr<ThreadSafeModule> tsm5 = GetThreadSafeModuleFromBitcodeStub(sig5);

    using _Sig12Proto = int(*)(int, int);
    using _Sig34Proto = int(*)(int);
    using _Sig5Proto = double(*)(double);

    jit.SetNonAstModule(std::move(tsm1));
    _Sig12Proto fn1 = jit.GetFunctionNonAst<_Sig12Proto>(std::string(sig1.m_symbolName));
    for (int i = 0; i < 100; i++)
    {
        int x = rand(), y = rand();
        ReleaseAssert(fn1(x, y) == x + y + 4);
    }

    jit.SetNonAstModule(std::move(tsm2));
    _Sig12Proto fn2 = jit.GetFunctionNonAst<_Sig12Proto>(std::string(sig2.m_symbolName));
    for (int i = 0; i < 100; i++)
    {
        int x = rand(), y = rand();
        ReleaseAssert(fn2(x, y) == x + y);
    }

    jit.SetNonAstModule(std::move(tsm3));
    _Sig34Proto fn3 = jit.GetFunctionNonAst<_Sig34Proto>(std::string(sig3.m_symbolName));
    for (int i = 0; i < 100; i++)
    {
        int x = rand();
        ReleaseAssert(fn3(x) == x + 7);
    }

    jit.SetNonAstModule(std::move(tsm4));
    _Sig34Proto fn4 = jit.GetFunctionNonAst<_Sig34Proto>(std::string(sig4.m_symbolName));
    for (int i = 0; i < 100; i++)
    {
        int x = rand();
        ReleaseAssert(fn4(x) == x + 3);
    }

    jit.SetNonAstModule(std::move(tsm5));
    _Sig5Proto fn5 = jit.GetFunctionNonAst<_Sig5Proto>(std::string(sig5.m_symbolName));
    for (int i = 0; i < 100; i++)
    {
        int x = rand() % 1000000;
        double dx = double(x) / 1000;
        double out = fn5(dx);
        ReleaseAssert(fabs(dx - out + 2.3) < 1e-8);
    }
}

TEST(SanityBitcode, SanityLink)
{
    const BitcodeData& getY = __pochivm_internal_bc_fb2d7a235b0bee90ed026467;
    const BitcodeData& setY = __pochivm_internal_bc_9e962f83bf07fe71b8a356ec;
    const BitcodeData& getXPlusY = __pochivm_internal_bc_0168482cbf35dd60c2677b26;
    const BitcodeData& getStringY = __pochivm_internal_bc_888a3d0065471b9b797cc710;

    const BitcodeData& pushVec = __pochivm_internal_bc_9d40e07b8417ba7543c9b4cd;
    const BitcodeData& sortVector = __pochivm_internal_bc_9211d742c8c7612d69d11c5b;
    const BitcodeData& getVectorSize = __pochivm_internal_bc_ece726fdd4130f1f904091ef;
    const BitcodeData& getVectorSum = __pochivm_internal_bc_2d9dcbc4a90b11f2fd8ca7d6;

    // int FreeFunctionOverloaded<double, double, int>(int, int)
    const BitcodeData& sig1 = __pochivm_internal_bc_bd614a9c9c6a469d1aa5a33c;
    // int FreeFunctionOverloaded<double, double, double>(int, int)
    const BitcodeData& sig2 = __pochivm_internal_bc_a9b7de4eb4a4edee827deb07;
    // int FreeFunctionOverloaded<int, int, int>(int)
    const BitcodeData& sig3 = __pochivm_internal_bc_55fe00ceaaa5ab7945195db3;
    // int FreeFunctionOverloaded<int, int, double>(int)
    const BitcodeData& sig4 = __pochivm_internal_bc_6d29f5afd61bc9b4312e126b;
    // double FreeFunctionOverloaded(double)
    const BitcodeData& sig5 = __pochivm_internal_bc_0c6d6b096143d90aa39723f9;

    std::unique_ptr<LLVMContext> context(new LLVMContext);
    std::unique_ptr<Module> module(new Module("test", *context.get()));

    LinkinBitcodeStub(*module.get(), getY);
    LinkinBitcodeStub(*module.get(), setY);
    LinkinBitcodeStub(*module.get(), getXPlusY);
    LinkinBitcodeStub(*module.get(), getStringY);
    LinkinBitcodeStub(*module.get(), pushVec);
    LinkinBitcodeStub(*module.get(), sortVector);
    LinkinBitcodeStub(*module.get(), getVectorSize);
    LinkinBitcodeStub(*module.get(), getVectorSum);
    LinkinBitcodeStub(*module.get(), sig1);
    LinkinBitcodeStub(*module.get(), sig2);
    LinkinBitcodeStub(*module.get(), sig3);
    LinkinBitcodeStub(*module.get(), sig4);
    LinkinBitcodeStub(*module.get(), sig5);

    TestClassA a;
    a.m_y = 0;

    SimpleJIT jit;
    jit.SetAllowResolveSymbolInHostProcess(true);
    jit.SetNonAstModule(std::unique_ptr<ThreadSafeModule>(new ThreadSafeModule(std::move(module), std::move(context))));

    using _SetFnPrototype = void(*)(TestClassA*, int);
    _SetFnPrototype setFn = jit.GetFunctionNonAst<_SetFnPrototype>(std::string(setY.m_symbolName));
    // make sure we are getting the pointer to generated code, not host process
    //
    ReleaseAssert(reinterpret_cast<void*>(setFn) != AstTypeHelper::GetClassMethodPtr(&TestClassA::SetY));

    using _GetFnPrototype = int(*)(TestClassA*);
    _GetFnPrototype getFn = jit.GetFunctionNonAst<_GetFnPrototype>(std::string(getY.m_symbolName));

    using _GetXPlusYFnPrototype = int(*)(TestClassA*, int);
    _GetXPlusYFnPrototype getXPlusYFn = jit.GetFunctionNonAst<_GetXPlusYFnPrototype>(std::string(getXPlusY.m_symbolName));

    using _GetStringYFnPrototype = std::string(*)(TestClassA*);
    _GetStringYFnPrototype getStringYFn = jit.GetFunctionNonAst<_GetStringYFnPrototype>(std::string(getStringY.m_symbolName));

    using _PushFnPrototype = void(*)(TestClassA*, int);
    _PushFnPrototype pushFn = jit.GetFunctionNonAst<_PushFnPrototype>(std::string(pushVec.m_symbolName));

    using _SortFnPrototype = int(*)(TestClassA*);
    _SortFnPrototype sortFn = jit.GetFunctionNonAst<_SortFnPrototype>(std::string(sortVector.m_symbolName));

    using _GetSizeFnPrototype = size_t(*)(TestClassA*);
    _GetSizeFnPrototype getSizeFn = jit.GetFunctionNonAst<_GetSizeFnPrototype>(std::string(getVectorSize.m_symbolName));

    using _GetSumFnPrototype = int64_t(*)(TestClassA*);
    _GetSumFnPrototype getSumFn = jit.GetFunctionNonAst<_GetSumFnPrototype>(std::string(getVectorSum.m_symbolName));

    using _Sig12Proto = int(*)(int, int);
    _Sig12Proto fn1 = jit.GetFunctionNonAst<_Sig12Proto>(std::string(sig1.m_symbolName));
    _Sig12Proto fn2 = jit.GetFunctionNonAst<_Sig12Proto>(std::string(sig2.m_symbolName));

    using _Sig34Proto = int(*)(int);
    _Sig34Proto fn3 = jit.GetFunctionNonAst<_Sig34Proto>(std::string(sig3.m_symbolName));
    _Sig34Proto fn4 = jit.GetFunctionNonAst<_Sig34Proto>(std::string(sig4.m_symbolName));

    using _Sig5Proto = double(*)(double);
    _Sig5Proto fn5 = jit.GetFunctionNonAst<_Sig5Proto>(std::string(sig5.m_symbolName));

    setFn(&a, 233);
    ReleaseAssert(a.m_y == 233);

    {
        int r = getFn(&a);
        ReleaseAssert(r == 233);
        ReleaseAssert(a.m_y == 233);
    }

    {
        int r = getXPlusYFn(&a, 456);
        ReleaseAssert(r == 233 + 456);
        ReleaseAssert(a.m_y == 233);
    }

    {
        std::string r = getStringYFn(&a);
        ReleaseAssert(r == "mystr_233");
    }

    for (size_t i = 0; i < 97; i++)
    {
        pushFn(&a, i * 37 % 97);
        ReleaseAssert(a.m_vec.size() == i + 1);
        for (size_t j = 0; j < i; j++)
        {
            ReleaseAssert(a.m_vec[j] == j * 37 % 97);
        }
    }

    for (int k = 0; k < 2; k++)
    {
        sortFn(&a);
        ReleaseAssert(a.m_vec.size() == 97);
        for (size_t i = 0; i < 97; i++)
        {
            ReleaseAssert(a.m_vec[i] == static_cast<int>(i));
        }
    }

    for (int k = 0; k < 2; k++)
    {
        ReleaseAssert(a.m_vec.size() == 97);
        size_t r = getSizeFn(&a);
        ReleaseAssert(r == 97);
    }

    for (int k = 0; k < 2; k++)
    {
        ReleaseAssert(a.m_vec.size() == 97);
        int64_t r = getSumFn(&a);
        ReleaseAssert(r == 97 * 96 / 2);
    }

    for (int i = 0; i < 100; i++)
    {
        int x = rand(), y = rand();
        ReleaseAssert(fn1(x, y) == x + y + 4);
    }

    for (int i = 0; i < 100; i++)
    {
        int x = rand(), y = rand();
        ReleaseAssert(fn2(x, y) == x + y);
    }

    for (int i = 0; i < 100; i++)
    {
        int x = rand();
        ReleaseAssert(fn3(x) == x + 7);
    }

    for (int i = 0; i < 100; i++)
    {
        int x = rand();
        ReleaseAssert(fn4(x) == x + 3);
    }

    for (int i = 0; i < 100; i++)
    {
        int x = rand() % 1000000;
        double dx = double(x) / 1000;
        double out = fn5(dx);
        ReleaseAssert(fabs(dx - out + 2.3) < 1e-8);
    }
}

namespace {

void CheckBitCodeData(const BitcodeData& data)
{
    std::unique_ptr<LLVMContext> context(new LLVMContext);
    std::unique_ptr<Module> module = GetModuleFromBitcodeStub(data, context.get());

    std::string _dst;
    llvm::raw_string_ostream rso(_dst /*target*/);
    module->print(rso, nullptr);
    std::string& dump = rso.str();

    AssertIsExpectedOutput(dump);
}

}

TEST(SanityBitCode, Inlining1_Debug)
{
    if (x_isDebugBuild)
    {
        const BitcodeData& fn = __pochivm_internal_bc_0e9e560e773403ef605b0d2c;
        CheckBitCodeData(fn);
    }
}

TEST(SanityBitCode, Inlining1_Release)
{
    if (!x_isDebugBuild)
    {
        const BitcodeData& fn = __pochivm_internal_bc_0e9e560e773403ef605b0d2c;
        CheckBitCodeData(fn);
    }
}

TEST(SanityBitCode, Inlining2_Debug)
{
    if (x_isDebugBuild)
    {
        const BitcodeData& fn = __pochivm_internal_bc_31a6bfa0434503770ac61589;
        CheckBitCodeData(fn);
    }
}

TEST(SanityBitCode, Inlining2_Release)
{
    if (!x_isDebugBuild)
    {
        const BitcodeData& fn = __pochivm_internal_bc_31a6bfa0434503770ac61589;
        CheckBitCodeData(fn);
    }
}

TEST(SanityBitCode, Inlining3_Debug)
{
    if (x_isDebugBuild)
    {
        const BitcodeData& fn = __pochivm_internal_bc_541d1d383e9f03720e359206;
        CheckBitCodeData(fn);
    }
}

TEST(SanityBitCode, Inlining3_Release)
{
    if (!x_isDebugBuild)
    {
        const BitcodeData& fn = __pochivm_internal_bc_541d1d383e9f03720e359206;
        CheckBitCodeData(fn);
    }
}

TEST(SanityBitCode, SanityInliningWorks)
{
    if (!x_isDebugBuild)
    {
        const BitcodeData& fn = __pochivm_internal_bc_0e9e560e773403ef605b0d2c;
        std::unique_ptr<LLVMContext> context(new LLVMContext);
        std::unique_ptr<Module> module = GetModuleFromBitcodeStub(fn, context.get());

        AutoThreadPochiVMContext apv;
        AutoThreadErrorContext arc;
        AutoThreadLLVMCodegenContext alc;

        thread_llvmContext->m_llvmContext = context.get();
        thread_llvmContext->RunOptimizationPass(module.get(), 3 /*optLevel*/);

        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        module->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump);
    }
}

TEST(SanityBitCode, SanityInliningWorks_2)
{
    if (!x_isDebugBuild)
    {
        const BitcodeData& fn = __pochivm_internal_bc_541d1d383e9f03720e359206;
        std::unique_ptr<LLVMContext> context(new LLVMContext);
        std::unique_ptr<Module> module = GetModuleFromBitcodeStub(fn, context.get());
        {
            // change linkage to available_externally
            //
            llvm::Function* func = module->getFunction(fn.m_symbolName);
            ReleaseAssert(func != nullptr);
            ReleaseAssert(func->getLinkage() == GlobalValue::LinkageTypes::ExternalLinkage);
            func->setLinkage(GlobalValue::LinkageTypes::AvailableExternallyLinkage);
        }

        llvm::IRBuilder<>* irBuilder = new llvm::IRBuilder<>(*context.get());

        const char* llvmType = TypeId::Get<TestSmallClass>().GetCppTypeLLVMTypeName();
        StructType* stype = module->getTypeByName(llvmType);
        ReleaseAssert(stype != nullptr);

        Type* paramType = stype->getPointerTo();
        Type* paramTypeArray[1] = { paramType };
        FunctionType* funcType = FunctionType::get(Type::getInt32Ty(*context.get()),
                                                   ArrayRef<Type*>(paramTypeArray, paramTypeArray + 1),
                                                   false /*isVariadic*/);

        llvm::Function* func = llvm::Function::Create(
                funcType, llvm::Function::ExternalLinkage, "testfn", module.get());

        BasicBlock* body = BasicBlock::Create(*context.get(), "body", func);
        irBuilder->SetInsertPoint(body);

        llvm::Function* callee = module->getFunction(fn.m_symbolName);
        ReleaseAssert(callee != nullptr);
        ReleaseAssert(callee->arg_size() == 2);

        llvm::Value* params[2];
        for (size_t index = 0; index < 1; index++)
        {
            params[index] = func->getArg(0);
        }
        params[1] = ConstantInt::get(*context.get(), APInt(32 /*numBits*/, 1 /*value*/, true /*isSigned*/));
        llvm::Value* r = nullptr;
        for (int i = 0; i < 10; i++)
        {
            r = irBuilder->CreateCall(callee, ArrayRef<llvm::Value*>(params, params + 2));
            params[1] = r;
        }
        irBuilder->CreateRet(r);

        ReleaseAssert(llvm::verifyFunction(*func, &outs()) == false);

        AutoThreadPochiVMContext apv;
        AutoThreadErrorContext arc;
        AutoThreadLLVMCodegenContext alc;

        thread_llvmContext->SetupModule(context.get(), irBuilder, module.get());
        thread_llvmContext->RunOptimizationPass(module.get(), 3 /*optLevel*/);

        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        module->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump);
    }
}

TEST(SanityBitCode, SanityNotInlinableFunction)
{
    if (!x_isDebugBuild)
    {
        // FreeFnRecursive
        //
        const BitcodeData& fn = __pochivm_internal_bc_f1b5fa59b278ee9cbb1502aa;
        std::unique_ptr<LLVMContext> context(new LLVMContext);
        std::unique_ptr<Module> module = GetModuleFromBitcodeStub(fn, context.get());
        {
            // change linkage to available_externally
            //
            llvm::Function* func = module->getFunction(fn.m_symbolName);
            ReleaseAssert(func != nullptr);
            ReleaseAssert(func->getLinkage() == GlobalValue::LinkageTypes::ExternalLinkage);
            func->setLinkage(GlobalValue::LinkageTypes::AvailableExternallyLinkage);
        }

        llvm::IRBuilder<>* irBuilder = new llvm::IRBuilder<>(*context.get());

        Type* paramTypeArray[2] = { Type::getInt1Ty(*context.get()), Type::getInt32Ty(*context.get()) };
        FunctionType* funcType = FunctionType::get(Type::getInt32Ty(*context.get()),
                                                   ArrayRef<Type*>(paramTypeArray, paramTypeArray + 2),
                                                   false /*isVariadic*/);

        llvm::Function* func = llvm::Function::Create(
                funcType, llvm::Function::ExternalLinkage, "testfn", module.get());

        BasicBlock* body = BasicBlock::Create(*context.get(), "body", func);
        BasicBlock* path1 = BasicBlock::Create(*context.get(), "path1", func);
        BasicBlock* path2 = BasicBlock::Create(*context.get(), "path2", func);
        irBuilder->SetInsertPoint(body);
        irBuilder->CreateCondBr(func->getArg(0), path1, path2);

        irBuilder->SetInsertPoint(path1);
        llvm::Function* callee = module->getFunction(fn.m_symbolName);
        ReleaseAssert(callee != nullptr);
        ReleaseAssert(callee->arg_size() == 1);

        llvm::Value* params[1];
        params[0] = func->getArg(1);

        llvm::Value* r = irBuilder->CreateCall(callee, ArrayRef<llvm::Value*>(params, params + 1));
        irBuilder->CreateRet(r);

        irBuilder->SetInsertPoint(path2);
        irBuilder->CreateRet(ConstantInt::get(*context.get(), APInt(32 /*numBits*/, 0 /*value*/, true /*isSigned*/)));

        ReleaseAssert(llvm::verifyFunction(*func, &outs()) == false);

        AutoThreadPochiVMContext apv;
        AutoThreadErrorContext arc;
        AutoThreadLLVMCodegenContext alc;

        thread_llvmContext->SetupModule(context.get(), irBuilder, module.get());
        thread_llvmContext->RunOptimizationPass(module.get(), 3 /*optLevel*/);

        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        module->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump);
    }
}

TEST(SanityBitCode, SanityNotInlinableFunction_2)
{
    if (!x_isDebugBuild)
    {
        // FreeFnRecursive2
        //
        const BitcodeData& fn = __pochivm_internal_bc_d9eab28a0482d30ba19cd1dd;
        std::unique_ptr<LLVMContext> context(new LLVMContext);
        std::unique_ptr<Module> module = GetModuleFromBitcodeStub(fn, context.get());
        {
            // change linkage to available_externally
            //
            llvm::Function* func = module->getFunction(fn.m_symbolName);
            ReleaseAssert(func != nullptr);
            ReleaseAssert(func->getLinkage() == GlobalValue::LinkageTypes::ExternalLinkage);
            func->setLinkage(GlobalValue::LinkageTypes::AvailableExternallyLinkage);
        }

        llvm::IRBuilder<>* irBuilder = new llvm::IRBuilder<>(*context.get());

        const char* llvmType = TypeId::Get<std::string>().GetCppTypeLLVMTypeName();
        StructType* stype = module->getTypeByName(llvmType);
        ReleaseAssert(stype != nullptr);
        Type* paramType = stype->getPointerTo();

        Type* paramTypeArray[2] = { Type::getInt1Ty(*context.get()), paramType };
        FunctionType* funcType = FunctionType::get(Type::getInt32Ty(*context.get()),
                                                   ArrayRef<Type*>(paramTypeArray, paramTypeArray + 2),
                                                   false /*isVariadic*/);

        llvm::Function* func = llvm::Function::Create(
                funcType, llvm::Function::ExternalLinkage, "testfn", module.get());

        BasicBlock* body = BasicBlock::Create(*context.get(), "body", func);
        BasicBlock* path1 = BasicBlock::Create(*context.get(), "path1", func);
        BasicBlock* path2 = BasicBlock::Create(*context.get(), "path2", func);
        irBuilder->SetInsertPoint(body);
        irBuilder->CreateCondBr(func->getArg(0), path1, path2);

        irBuilder->SetInsertPoint(path1);
        llvm::Function* callee = module->getFunction(fn.m_symbolName);
        ReleaseAssert(callee != nullptr);
        ReleaseAssert(callee->arg_size() == 1);

        llvm::Value* params[1];
        params[0] = func->getArg(1);

        llvm::Value* r = irBuilder->CreateCall(callee, ArrayRef<llvm::Value*>(params, params + 1));
        irBuilder->CreateRet(r);

        irBuilder->SetInsertPoint(path2);
        irBuilder->CreateRet(ConstantInt::get(*context.get(), APInt(32 /*numBits*/, 0 /*value*/, true /*isSigned*/)));

        ReleaseAssert(llvm::verifyFunction(*func, &outs()) == false);

        AutoThreadPochiVMContext apv;
        AutoThreadErrorContext arc;
        AutoThreadLLVMCodegenContext alc;

        thread_llvmContext->SetupModule(context.get(), irBuilder, module.get());
        thread_llvmContext->RunOptimizationPass(module.get(), 3 /*optLevel*/);

        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        module->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump);

        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetNonAstModule(std::unique_ptr<ThreadSafeModule>(new ThreadSafeModule(std::move(module), std::move(context))));

        using _FnPrototype = int(*)(bool, std::string*);
        _FnPrototype testfn = jit.GetFunctionNonAst<_FnPrototype>("testfn");

        std::string input = "10";
        int result = testfn(true, &input);
        ReleaseAssert(result == 89);

        result = testfn(false, &input);
        ReleaseAssert(result == 0);
    }
}

TEST(SanityBitCode, SanityNotInlinableFunction_3)
{
    if (!x_isDebugBuild)
    {
        // FreeFnRecursive3
        //
        const BitcodeData& fn = __pochivm_internal_bc_99204e48f216121e0ebdb948;
        std::unique_ptr<LLVMContext> context(new LLVMContext);
        std::unique_ptr<Module> module = GetModuleFromBitcodeStub(fn, context.get());
        {
            // change linkage to available_externally
            //
            llvm::Function* func = module->getFunction(fn.m_symbolName);
            ReleaseAssert(func != nullptr);
            ReleaseAssert(func->getLinkage() == GlobalValue::LinkageTypes::ExternalLinkage);
            func->setLinkage(GlobalValue::LinkageTypes::AvailableExternallyLinkage);
        }

        llvm::IRBuilder<>* irBuilder = new llvm::IRBuilder<>(*context.get());

        const char* llvmType = TypeId::Get<std::string>().GetCppTypeLLVMTypeName();
        StructType* stype = module->getTypeByName(llvmType);
        ReleaseAssert(stype != nullptr);
        Type* paramType = stype->getPointerTo();

        Type* paramTypeArray[3] = { Type::getInt1Ty(*context.get()), paramType, paramType };
        FunctionType* funcType = FunctionType::get(Type::getVoidTy(*context.get()),
                                                   ArrayRef<Type*>(paramTypeArray, paramTypeArray + 3),
                                                   false /*isVariadic*/);

        llvm::Function* func = llvm::Function::Create(
                funcType, llvm::Function::ExternalLinkage, "testfn", module.get());

        BasicBlock* body = BasicBlock::Create(*context.get(), "body", func);
        BasicBlock* path1 = BasicBlock::Create(*context.get(), "path1", func);
        BasicBlock* path2 = BasicBlock::Create(*context.get(), "path2", func);
        irBuilder->SetInsertPoint(body);
        irBuilder->CreateCondBr(func->getArg(0), path1, path2);

        irBuilder->SetInsertPoint(path1);
        llvm::Function* callee = module->getFunction(fn.m_symbolName);
        ReleaseAssert(callee != nullptr);
        ReleaseAssert(callee->arg_size() == 2);

        llvm::Value* params[2];
        params[0] = func->getArg(2);
        params[1] = func->getArg(1);

        irBuilder->CreateCall(callee, ArrayRef<llvm::Value*>(params, params + 2));
        irBuilder->CreateRetVoid();

        irBuilder->SetInsertPoint(path2);
        irBuilder->CreateRetVoid();

        ReleaseAssert(llvm::verifyFunction(*func, &outs()) == false);

        AutoThreadPochiVMContext apv;
        AutoThreadErrorContext arc;
        AutoThreadLLVMCodegenContext alc;

        thread_llvmContext->SetupModule(context.get(), irBuilder, module.get());
        thread_llvmContext->RunOptimizationPass(module.get(), 3 /*optLevel*/);

        std::string _dst;
        llvm::raw_string_ostream rso(_dst /*target*/);
        module->print(rso, nullptr);
        std::string& dump = rso.str();

        AssertIsExpectedOutput(dump);

        SimpleJIT jit;
        jit.SetAllowResolveSymbolInHostProcess(true);
        jit.SetNonAstModule(std::unique_ptr<ThreadSafeModule>(new ThreadSafeModule(std::move(module), std::move(context))));

        using _FnPrototype = void(*)(bool, std::string*, std::string* /*out*/);
        _FnPrototype testfn = jit.GetFunctionNonAst<_FnPrototype>("testfn");

        std::string prefix = std::string(100, '_');
        ReleaseAssert(prefix.length() == 100);

        {
            std::string input = prefix + "10";
            std::string* output = reinterpret_cast<std::string*>(alloca(sizeof(std::string)));
            memset(output, 0, sizeof(std::string));
            testfn(true, &input, output);
            ReleaseAssert(*output == prefix + "89");
        }

        {
            std::string input = prefix + "10";
            std::string* output = reinterpret_cast<std::string*>(alloca(sizeof(std::string)));
            memset(output, 0, sizeof(std::string));
            testfn(false, &input, output);
            uint8_t* raw = reinterpret_cast<uint8_t*>(output);
            for (size_t i = 0; i < sizeof(std::string); i++)
            {
                ReleaseAssert(raw[i] == 0);
            }
        }
    }
}
