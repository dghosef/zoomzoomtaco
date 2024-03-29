#define POCHIVM_INSIDE_FASTINTERP_TPL_CPP

#include "fastinterp_tpl_common.hpp"
#include "fastinterp_tpl_operandshape.hpp"
#include "fastinterp_tpl_arith_operator_helper.hpp"

namespace PochiVM
{

// partially inlined dereference impl
// var[var/lit] [%]
//
struct FIPartialInlineLhsDereferenceImpl
{
    template<typename OperandType>
    static constexpr bool cond()
    {
        if (std::is_same<OperandType, void>::value) { return false; }
        if (std::is_pointer<OperandType>::value && !std::is_same<OperandType, void*>::value) { return false; }
        return true;
    }

    template<typename OperandType,
             typename IndexType>
    static constexpr bool cond()
    {
        if (!is_valid_index_type<IndexType>()) { return false; }
        return true;
    }

    template<typename OperandType,
             typename IndexType,
             typename IndexOperandType>
    static constexpr bool cond()
    {
        if (std::is_same<IndexOperandType, void>::value ||
            std::is_floating_point<IndexOperandType>::value ||
            std::is_pointer<IndexOperandType>::value)
        {
            return false;
        }
        return true;
    }

    template<typename OperandType,
             typename IndexType,
             typename IndexOperandType,
             FIOperandShapeCategory shapeCategory>
    static constexpr bool cond()
    {
        if (!FIOperandShapeCategoryHelper::cond<OperandType, IndexType, shapeCategory>()) { return false; }
        if (shapeCategory == FIOperandShapeCategory::ZERO || shapeCategory == FIOperandShapeCategory::LITERAL_NONZERO) { return false; }
        return true;
    }

    template<typename OperandType,
             typename IndexType,
             typename IndexOperandType,
             FIOperandShapeCategory shapeCategory,
             bool spillOutput,
             FINumOpaqueIntegralParams numOIP>
    static constexpr bool cond()
    {
        if (!FIOpaqueParamsHelper::CanPush(numOIP))
        {
            return false;
        }
        return true;
    }

    template<typename OperandType,
             typename IndexType,
             typename IndexOperandType,
             FIOperandShapeCategory shapeCategory,
             bool spillOutput,
             FINumOpaqueIntegralParams numOIP,
             FINumOpaqueFloatingParams numOFP>
    static constexpr bool cond()
    {
        if (!spillOutput && std::is_floating_point<OperandType>::value)
        {
            if (!FIOpaqueParamsHelper::CanPush(numOFP)) { return false; }
        }
        else
        {
            if (FIOpaqueParamsHelper::CanPush(numOFP)) { return false; }
        }
        return true;
    }

    // placeholder rules:
    // constant placeholder 0: spill position, if spillOutput
    // constant placeholder 1/2: inlined operand shape
    //
    template<typename OperandType,
             typename IndexType,
             typename IndexOperandType,
             FIOperandShapeCategory shapeCategory,
             bool spillOutput,
             FINumOpaqueIntegralParams numOIP,
             FINumOpaqueFloatingParams numOFP,
             typename... OpaqueParams>
    static void f(uintptr_t stackframe, DEF_MEM2REG_PARAMS, OpaqueParams... opaqueParams, IndexOperandType qa) noexcept
    {
        OperandType* base = FIOperandShapeCategoryHelper::get_1_2<OperandType*, IndexType, shapeCategory>(stackframe);

        OperandType result = base[qa];

        if constexpr(!spillOutput)
        {
            DEFINE_BOILERPLATE_FNPTR_PLACEHOLDER_0(void(*)(uintptr_t, MEM2REG_TYPES, OpaqueParams..., OperandType) noexcept);
            BOILERPLATE_FNPTR_PLACEHOLDER_0(stackframe, PASS_MEM2REG_PARAMS, opaqueParams..., result);
        }
        else
        {
            DEFINE_INDEX_CONSTANT_PLACEHOLDER_0;
            *GetLocalVarAddress<OperandType>(stackframe, CONSTANT_PLACEHOLDER_0) = result;

            DEFINE_BOILERPLATE_FNPTR_PLACEHOLDER_0(void(*)(uintptr_t, MEM2REG_TYPES, OpaqueParams...) noexcept);
            BOILERPLATE_FNPTR_PLACEHOLDER_0(stackframe, PASS_MEM2REG_PARAMS, opaqueParams...);
        }
    }

    static auto metavars()
    {
        return CreateMetaVarList(
                    CreateTypeMetaVar("operandType"),
                    CreateTypeMetaVar("indexType"),
                    CreateTypeMetaVar("indexOperandType"),
                    CreateEnumMetaVar<FIOperandShapeCategory::X_END_OF_ENUM>("shapeCategory"),
                    CreateBoolMetaVar("spillOutput"),
                    CreateOpaqueIntegralParamsLimit(),
                    CreateOpaqueFloatParamsLimit()
        );
    }
};

}   // namespace PochiVM

// build_fast_interp_lib.cpp JIT entry point
//
extern "C"
void __pochivm_build_fast_interp_library__()
{
    using namespace PochiVM;
    RegisterBoilerplate<FIPartialInlineLhsDereferenceImpl>();
}
