
#define POCHIVM_INSIDE_FASTINTERP_TPL_CPP

#include "fastinterp_tpl_common.hpp"
#include "fastinterp_tpl_mem2reg_util.hpp"

namespace PochiVM
{

struct FIMem2RegInt32LT
{
    template<FIMem2RegOrdinal ordinal,
             FIMem2RegOrdinal ordinal2,
             bool spillOutput,
             FINumOpaqueIntegralParams numOIP,
             FINumOpaqueFloatingParams numOFP>
    static constexpr bool cond()
    {
        // TODO: Add numoip and numofp checks
        if (static_cast<size_t>(ordinal) >= x_mem2reg_max_integral_vars) { return false; }
        return true;
    }

    
    template<FIMem2RegOrdinal ordinal,
             FIMem2RegOrdinal ordinal2,
             bool spillOutput,
             FINumOpaqueIntegralParams numOIP,
             FINumOpaqueFloatingParams numOFP,
             typename... OpaqueParams>
    static void f(uintptr_t stackframe,
                  GEN_MEM2REG_EXTRACT_DEFS(true, static_cast<size_t>(ordinal), uint64_t),
                  OpaqueParams... opaqueParams) noexcept
    {
        constexpr size_t ord1 = static_cast<size_t>(ordinal);
        constexpr size_t ord2 = static_cast<size_t>(ordinal2);
        bool result = static_cast<int32_t>(EXTRACT_MEM2REG_VALUE(true, ord1)) < static_cast<int32_t>(EXTRACT_MEM2REG_VALUE(true, ord2));
        if constexpr(!spillOutput)
        {
            DEFINE_BOILERPLATE_FNPTR_PLACEHOLDER_0(void(*)(uintptr_t, GEN_MEM2REG_EXTRACT_TYPES(true, ord1, uint64_t), OpaqueParams..., bool) noexcept);
            BOILERPLATE_FNPTR_PLACEHOLDER_0(stackframe, PASS_MEM2REG_PARAMS, opaqueParams..., result);
        }
        else
        {
            DEFINE_INDEX_CONSTANT_PLACEHOLDER_0;
            *GetLocalVarAddress<bool>(stackframe, CONSTANT_PLACEHOLDER_0) = result;

            DEFINE_BOILERPLATE_FNPTR_PLACEHOLDER_0(void(*)(uintptr_t, GEN_MEM2REG_EXTRACT_TYPES(true, ord1, uint64_t), OpaqueParams...) noexcept);
            BOILERPLATE_FNPTR_PLACEHOLDER_0(stackframe, PASS_MEM2REG_PARAMS, opaqueParams...);
        }
    }

    static auto metavars()
    {
        return CreateMetaVarList(
                    CreateEnumMetaVar<FIMem2RegOrdinal::X_END_OF_ENUM>("ordinal"),
                    CreateEnumMetaVar<FIMem2RegOrdinal::X_END_OF_ENUM>("ordinal2"),
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
    RegisterBoilerplate<FIMem2RegInt32LT>();
}
