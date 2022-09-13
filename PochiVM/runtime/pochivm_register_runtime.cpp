// DO NOT INCLUDE ADDITIONAL HEADERS
// Put the includes in 'pochivm_runtime_headers.h', not here.
//
#include "pochivm_runtime_headers.h"
#include "pochivm/pochivm_reflection_helper.h"

__attribute__((__optnone__))
static void RegisterRuntimeLibrary()
{
    using namespace PochiVM;

    // ****************************************
    // Register the list of functions callable from generated code, using the APIs below.
    //
    //    void RegisterFreeFn<function pointer>()
    //    void RegisterMemberFn<member function pointer>()
    //    void RegisterStaticMemberFn<member function pointer>()
    //    void RegisterMemberObject<member object pointer>()
    //    void RegisterConstructor<ClassName, ArgTypeNames...>()
    //    void RegisterExceptionObjectType<Type>()
    //
    // While you may write any logic you like, keep in mind that you will get a segfault if you try to
    // call functions or access global values which implementations reside in other CPP files.
    // E.g. suppose implementation of function 'f' is not in this file.
    //      'auto fnPtr = &f' or 'RegisterFreeFn<&f>()' is OK since it only takes address of the symbol
    //      (which does not require knowledge of its implementation). But 'f()' will give you a segfault.
    //
    // ****************************************
    RegisterMemberFn<&TestClassA::GetY>();
    RegisterMemberFn<&TestClassA::SetY>();
    RegisterMemberFn<&TestClassA::GetXPlusY>();
    RegisterMemberFn<&TestClassA::PushVec>();
    RegisterMemberFn<&TestClassA::GetSize>();
    RegisterMemberFn<&TestClassA::SortVector>();
    RegisterMemberFn<&TestClassA::GetVectorSum>();
    RegisterMemberFn<&TestClassA::GetStringY>();

    RegisterMemberFn<&TestClassB::SetA>();
    RegisterMemberFn<&TestClassB::GetA>();
    RegisterMemberFn<&TestClassB::SetAp>();
    RegisterMemberFn<&TestClassB::GetAp>();
    RegisterMemberFn<&TestClassB::TestBool>();
    RegisterMemberFn<&TestClassB::TestBoolStar>();

    RegisterFreeFn<&FreeFunctionAPlusB>();
    RegisterFreeFn<&FreeFunctionStoreValue>();
    RegisterFreeFn<&FreeFunctionPrintFile>();
    RegisterFreeFn<&FreeFunctionConcatStr>();

    RegisterFreeFn<&FreeFunctionTemplated<int, 1>>();
    RegisterFreeFn<&FreeFunctionTemplated<int, 2>>();
    RegisterFreeFn<&FreeFunctionTemplated<int, 3>>();
    RegisterFreeFn<&FreeFunctionTemplated<double, 1>>();
    RegisterFreeFn<&FreeFunctionTemplated<double, 5>>();

    RegisterFreeFn<static_cast<int(*)(int)>(&FreeFunctionOverloaded<int, int, int>)>();
    RegisterFreeFn<static_cast<int(*)(int)>(&FreeFunctionOverloaded<int, int, double>)>();
    RegisterFreeFn<static_cast<int(*)(int)>(&FreeFunctionOverloaded<double, int, int>)>();
    RegisterFreeFn<static_cast<int(*)(int)>(&FreeFunctionOverloaded<int, double, double>)>();

    RegisterFreeFn<static_cast<int(*)(int, int)>(&FreeFunctionOverloaded<int, int, int>)>();
    RegisterFreeFn<static_cast<int(*)(int, int)>(&FreeFunctionOverloaded<int, double, int>)>();
    RegisterFreeFn<static_cast<int(*)(int, int)>(&FreeFunctionOverloaded<double, double, int>)>();
    RegisterFreeFn<static_cast<int(*)(int, int)>(&FreeFunctionOverloaded<double, double, double>)>();

    RegisterFreeFn<static_cast<double(*)(double)>(&FreeFunctionOverloaded)>();

    RegisterMemberFn<&TestLargeClass::F1>();
    RegisterMemberFn<&TestLargeClass::F2>();
    RegisterMemberFn<&TestLargeClass::F3>();
    RegisterMemberFn<&TestLargeClass::F4>();
    RegisterMemberFn<&TestLargeClass::F5>();

    RegisterMemberFn<&TestSmallClass::F1>();
    RegisterMemberFn<&TestSmallClass::F2>();
    RegisterMemberFn<&TestSmallClass::F3>();
    RegisterMemberFn<&TestSmallClass::F4>();
    RegisterMemberFn<&TestSmallClass::F5>();
    RegisterMemberFn<&TestSmallClass::F6>();
    RegisterMemberFn<&TestSmallClass::F7>();

    RegisterMemberFn<&TestSmallClass::G1>();
    RegisterMemberFn<&TestSmallClass::G2>();
    RegisterMemberFn<&TestSmallClass::G3>();
    RegisterMemberFn<&TestSmallClass::G4>();
    RegisterMemberFn<&TestSmallClass::G5>();
    RegisterMemberFn<&TestSmallClass::G6>();
    RegisterMemberFn<&TestSmallClass::G7>();

    RegisterStaticMemberFn<&TestSmallClass::S1>();
    RegisterStaticMemberFn<&TestSmallClass::S2>();

    RegisterMemberFn<static_cast<void(TestSmallClass::*)(int)>(&TestSmallClass::Overloaded)>();
    RegisterMemberFn<static_cast<void(TestSmallClass::*)(double)>(&TestSmallClass::Overloaded)>();
    RegisterMemberFn<static_cast<void(TestSmallClass::*)(TestSmallClass) const>(&TestSmallClass::Overloaded)>();

    RegisterFreeFn<&FreeFnTestSmallClass1>();
    RegisterFreeFn<&FreeFnTestSmallClass2>();
    RegisterFreeFn<&FreeFnTestSmallClass3>();
    RegisterFreeFn<&FreeFnTestSmallClass4>();
    RegisterFreeFn<&FreeFnTestSmallClass5>();

    RegisterFreeFn<&FreeFnTestLargeClass1>();
    RegisterFreeFn<&FreeFnTestLargeClass2>();
    RegisterFreeFn<&FreeFnTestLargeClass3>();
    RegisterFreeFn<&FreeFnTestLargeClass4>();

    RegisterFreeFn<&FreeFnRecursive>();
    RegisterFreeFn<&FreeFnRecursive2>();
    RegisterFreeFn<&FreeFnRecursive3>();

    RegisterFreeFn<&TestCornerCases::BoolParamTest1>();
    RegisterFreeFn<&TestCornerCases::BoolParamTest2>();
    RegisterFreeFn<&TestCornerCases::VoidStarParamTest1>();
    RegisterFreeFn<&TestCornerCases::VoidStarParamTest2>();

    RegisterStaticMemberFn<&TestNonTrivialConstructor::Create>();
    RegisterStaticMemberFn<&TestNonTrivialConstructor::Create2>();
    RegisterStaticMemberFn<&TestNonTrivialConstructor::Create3>();
    RegisterMemberFn<&TestNonTrivialConstructor::GetValue>();

    RegisterConstructor<TestNonTrivialCopyConstructor, int>();
    RegisterConstructor<TestNonTrivialCopyConstructor, const TestNonTrivialCopyConstructor&>();
    RegisterStaticMemberFn<&TestNonTrivialCopyConstructor::Fn>();

    RegisterConstructor<std::vector<int>>();
    RegisterConstructor<std::vector<int>, size_t /*count*/>();
    RegisterConstructor<std::vector<int>, size_t /*count*/, int /*value*/>();
    RegisterConstructor<std::vector<int>, const std::vector<int>& /*other*/>();

    RegisterConstructor<std::vector<uint64_t>>();
    RegisterConstructor<std::vector<uint64_t>, size_t /*count*/>();
    RegisterConstructor<std::vector<uint64_t>, size_t /*count*/, uint64_t /*value*/>();
    RegisterConstructor<std::vector<uint64_t>, const std::vector<uint64_t>& /*other*/>();

    RegisterFreeFn<&CopyVectorInt>();

    RegisterConstructor<TestConstructor1>();
    RegisterMemberFn<&TestConstructor1::GetValue>();

    RegisterConstructor<TestConstructor2, int>();
    RegisterMemberFn<&TestConstructor2::GetValue>();

    RegisterConstructor<TestDestructor1, int, int*>();

    RegisterConstructor<TestDestructor2, CtorDtorOrderRecorder*, int>();
    RegisterStaticMemberFn<&TestDestructor2::Create>();

    RegisterMemberFn<&CtorDtorOrderRecorder::Push>();
    RegisterMemberFn<&CtorDtorOrderRecorder::PushMaybeThrow>();

    RegisterFreeFn<&TestStaticVarInFunction>();
    RegisterFreeFn<&TestConstantWithSignificantAddress>();
    RegisterFreeFn<&TestConstantWithInsignificantAddress>();
    RegisterFreeFn<&StringInterningQuirkyBehavior>();

    RegisterFreeFn<&TestNoExceptButThrows>();

    RegisterExceptionObjectType<int>();
    RegisterExceptionObjectType<int*****>();
    RegisterExceptionObjectType<int*>();
    RegisterExceptionObjectType<const int*>();
    RegisterExceptionObjectType<const int* const*>();
    RegisterExceptionObjectType<std::exception>();
    RegisterExceptionObjectType<std::bad_alloc>();
    RegisterExceptionObjectType<std::vector<int>>();
    RegisterExceptionObjectType<std::vector<uint64_t>>();
    RegisterExceptionObjectType<TestNonTrivialCopyConstructor>();
    RegisterExceptionObjectType<TestNonTrivialConstructor>();
    RegisterExceptionObjectType<TestDestructor2>();

    RegisterConstructor<std::bad_alloc>();

    RegisterMemberObject<&std::pair<int, double>::first>();
    RegisterMemberObject<&std::pair<int, double>::second>();

    RegisterFreeFn<&TestMismatchedLLVMTypeName>();
    RegisterFreeFn<&TestMismatchedLLVMTypeName2>();
    RegisterFreeFn<&TestMismatchedLLVMTypeName3>();
    RegisterFreeFn<&TestMismatchedLLVMTypeName4>();

    RegisterMemberFn<static_cast<std::vector<int>::iterator(std::vector<int>::*)()>(&std::vector<int>::begin)>();
    RegisterMemberFn<static_cast<std::vector<int>::iterator(std::vector<int>::*)()>(&std::vector<int>::end)>();
    RegisterMemberFn<&std::vector<int>::iterator::operator*>();
    RegisterMemberFn<&std::vector<int>::iterator::operator->>();
    // RegisterMemberFn<static_cast<std::vector<int>::iterator(std::vector<int>::iterator::*)(int)>(&std::vector<int>::iterator::operator++)>();
    // RegisterMemberFn<static_cast<std::vector<int>::iterator&(std::vector<int>::iterator::*)()>(&std::vector<int>::iterator::operator++)>();
    RegisterMemberFn<static_cast<std::vector<uint64_t>::iterator(std::vector<uint64_t>::*)()>(&std::vector<uint64_t>::begin)>();
    RegisterMemberFn<static_cast<std::vector<uint64_t>::iterator(std::vector<uint64_t>::*)()>(&std::vector<uint64_t>::end)>();
    RegisterMemberFn<&std::vector<uint64_t>::iterator::operator*>();
    RegisterMemberFn<&std::vector<uint64_t>::iterator::operator->>();

    RegisterFreeFn<&TestConstPrimitiveTypeParam>();
    RegisterFreeFn<&TestConstPrimitiveTypeReturn1>();
    RegisterFreeFn<&TestConstPrimitiveTypeReturn2>();
    RegisterConstructor<TestConstPrimitiveTypeCtor, const int&, int&, const int*&, int*&, int* const&, const int* const&>();
    RegisterMemberObject<&TestConstPrimitiveTypeCtor::value>();
    RegisterFreeFn<&TestNonPrimitiveTypeConstRef>();
    RegisterFreeFn<&TestAddressOfConstPrimitiveRef>();

    RegisterMemberFn<&std::string::c_str>();
    RegisterMemberFn<&std::string::size>();
    RegisterMemberFn<static_cast<std::string::reference(std::string::*)(std::string::size_type)>(&std::string::operator[])>();
    RegisterConstructor<std::string, const char*>();

    RegisterMemberFn<&std::vector<std::string>::size>();
    RegisterMemberFn<static_cast<std::vector<std::string>::reference(std::vector<std::string>::*)(std::vector<std::string>::size_type)>(&std::vector<std::string>::operator[])>();

    RegisterMemberFn<static_cast<std::vector<std::string>::iterator(std::vector<std::string>::*)()>(&std::vector<std::string>::begin)>();
    RegisterMemberFn<static_cast<std::vector<std::string>::iterator(std::vector<std::string>::*)()>(&std::vector<std::string>::end)>();
    RegisterMemberFn<&std::vector<std::string>::iterator::operator*>();
    RegisterMemberFn<&std::vector<std::string>::iterator::operator->>();
    RegisterMemberFn<static_cast<std::vector<uint64_t>::iterator(std::vector<uint64_t>::*)()>(&std::vector<uint64_t>::begin)>();
    RegisterMemberFn<static_cast<std::vector<uint64_t>::iterator(std::vector<uint64_t>::*)()>(&std::vector<uint64_t>::end)>();

    RegisterOutlineDefinedOverloadedOperator<std::vector<std::string>::iterator, std::vector<std::string>::iterator, AstComparisonExprType::EQUAL>();
    RegisterOutlineDefinedOverloadedOperator<std::vector<std::string>::iterator, std::vector<std::string>::iterator, AstComparisonExprType::NOT_EQUAL>();
    RegisterOutlineDefinedOverloadedOperator<std::vector<std::string>::iterator, std::vector<std::string>::iterator, AstComparisonExprType::LESS_THAN>();
    RegisterOutlineDefinedOverloadedOperator<std::vector<uint64_t>::iterator, std::vector<uint64_t>::iterator, AstComparisonExprType::LESS_THAN>();
    RegisterOutlineDefinedOverloadedOperator<std::vector<std::string>::iterator, std::vector<std::string>::iterator, AstComparisonExprType::LESS_EQUAL>();
    RegisterOutlineDefinedOverloadedOperator<std::vector<std::string>::iterator, std::vector<std::string>::iterator, AstComparisonExprType::GREATER_THAN>();
    RegisterOutlineDefinedOverloadedOperator<std::vector<std::string>::iterator, std::vector<std::string>::iterator, AstComparisonExprType::GREATER_EQUAL>();

    RegisterOutlineIncrementOrDecrementOperator<std::vector<std::string>::iterator, true /*isIncrement*/>();
    RegisterOutlineIncrementOrDecrementOperator<std::vector<std::string>::iterator, false /*isIncrement*/>();

    RegisterConstructor<std::vector<uintptr_t>>();

    RegisterMemberFn<static_cast<std::vector<uintptr_t>::iterator(std::vector<uintptr_t>::*)()>(&std::vector<uintptr_t>::begin)>();
    RegisterMemberFn<static_cast<std::vector<uintptr_t>::iterator(std::vector<uintptr_t>::*)()>(&std::vector<uintptr_t>::end)>();
    RegisterMemberFn<&std::vector<uintptr_t>::iterator::operator*>();
    RegisterMemberFn<&std::vector<uintptr_t>::iterator::operator->>();

    RegisterOutlineDefinedOverloadedOperator<std::vector<uintptr_t>::iterator, std::vector<uintptr_t>::iterator, AstComparisonExprType::EQUAL>();
    RegisterOutlineDefinedOverloadedOperator<std::vector<uintptr_t>::iterator, std::vector<uintptr_t>::iterator, AstComparisonExprType::NOT_EQUAL>();
    RegisterOutlineDefinedOverloadedOperator<std::vector<uintptr_t>::iterator, std::vector<uintptr_t>::iterator, AstComparisonExprType::LESS_THAN>();
    RegisterOutlineDefinedOverloadedOperator<std::vector<uintptr_t>::iterator, std::vector<uintptr_t>::iterator, AstComparisonExprType::LESS_EQUAL>();
    RegisterOutlineDefinedOverloadedOperator<std::vector<uintptr_t>::iterator, std::vector<uintptr_t>::iterator, AstComparisonExprType::GREATER_THAN>();
    RegisterOutlineDefinedOverloadedOperator<std::vector<uintptr_t>::iterator, std::vector<uintptr_t>::iterator, AstComparisonExprType::GREATER_EQUAL>();

    RegisterOutlineIncrementOrDecrementOperator<std::vector<uintptr_t>::iterator, true /*isIncrement*/>();
    RegisterOutlineIncrementOrDecrementOperator<std::vector<uintptr_t>::iterator, false /*isIncrement*/>();

    RegisterMemberFn<static_cast<uintptr_t*(std::vector<uintptr_t>::*)()>(&std::vector<uintptr_t>::data)>();
    RegisterMemberFn<&std::vector<uintptr_t>::size>();
    RegisterMemberFn<static_cast<void(std::vector<uintptr_t>::*)(const uintptr_t&)>(&std::vector<uintptr_t>::push_back)>();

    RegisterConstructor<TestGeneratedFnPtr, uintptr_t>();
    RegisterMemberFn<&TestGeneratedFnPtr::execute>();

    RegisterMemberObject<&std::pair<const uintptr_t, uintptr_t>::first>();
    RegisterMemberObject<&std::pair<const uintptr_t, uintptr_t>::second>();

    RegisterConstructor<std::vector<uintptr_t>, size_t>();

    // For inlining benchmark, register functions that are guaranteed to never be inlined
#if 0
    RegisterFreeFn<&increment_wrapper>();
    RegisterFreeFn<&begin_wrapper>();
    RegisterFreeFn<&end_wrapper>();
    RegisterFreeFn<&data_wrapper>();
    RegisterFreeFn<&it_lt_wrapper>();
#endif

    /*************************************************************************
     * Taco registrations
     *************************************************************************/
    RegisterConstructor<taco_tensor_t>();
    RegisterMemberObject<&taco_tensor_t::vals>();
    RegisterMemberObject<&taco_tensor_t::vals_size>();
    RegisterMemberObject<&taco_tensor_t::dimensions>();
    RegisterMemberObject<&taco_tensor_t::indices>();
    RegisterConstructor<std::complex<float>>();
    RegisterConstructor<std::complex<double>>();
    RegisterConstructor<std::complex<long double>>();
    RegisterFreeFn<&omp_get_thread_num>();
    RegisterFreeFn<&omp_get_max_threads>();
    RegisterFreeFn<&cmp>();
    RegisterFreeFn<&taco_binarySearchAfter>();
    RegisterFreeFn<&taco_binarySearchBefore>();
    RegisterFreeFn<&init_taco_tensor_t>();
    RegisterFreeFn<&deinit_taco_tensor_t>();
    RegisterFreeFn<&pochi_calloc>();
    RegisterFreeFn<&pochi_malloc>();
    RegisterFreeFn<&pochi_realloc>();
    RegisterFreeFn<&pochi_free>();
    RegisterFreeFn<&pochi_qsort>();
    RegisterFreeFn<&pochi_bitand>();
    RegisterFreeFn<&pochi_bitor>();

    // Taco Math Registrations
    RegisterFreeFn<&sqrt_double>();
    RegisterFreeFn<&min>();
    RegisterFreeFn<&max>();
    #include "taco_register.h"
}

// DO NOT MODIFY
// dump_symbols.cpp JIT entry point
//
extern "C" __attribute__((__optnone__, __used__))
void __pochivm_register_runtime_library__()
{
    RegisterRuntimeLibrary();
}

