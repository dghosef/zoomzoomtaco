
#define POCHIVM_INSIDE_FASTINTERP_TPL_CPP

#include "fastinterp_tpl_common.hpp"

#define POCHIVM_INSIDE_FASTINTERP_TPL_CPP

#include "fastinterp_tpl_common.hpp"
#include "fastinterp_tpl_mem2reg_util.hpp"

namespace PochiVM
{

struct FIMem2RegInt32CompCondBranchImpl
{
    template<FIMem2RegOrdinal ordinal,
             FIMem2RegOrdinal ordinal2,
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
             FINumOpaqueIntegralParams numOIP,
             FINumOpaqueFloatingParams numOFP,
             typename... OpaqueParams>
    static void f(uintptr_t stackframe,
                  GEN_MEM2REG_EXTRACT_DEFS(true, static_cast<size_t>(ordinal), uint64_t),
                  OpaqueParams... opaqueParams) noexcept
    {
        constexpr size_t ord1 = static_cast<size_t>(ordinal);
        constexpr size_t ord2 = static_cast<size_t>(ordinal2);
        int32_t a = static_cast<int32_t>(EXTRACT_MEM2REG_VALUE(true, ord1));
        int32_t b = static_cast<int32_t>(EXTRACT_MEM2REG_VALUE(true, ord2));
        bool result = a < b;
        if (unlikely(result))                                                                                                \
        {                                                                                                                \
            INTERNAL_DEFINE_BOILERPLATE_FNPTR_PLACEHOLDER(0, void(*)(uintptr_t, MEM2REG_TYPES, OpaqueParams...) noexcept);   \
            BOILERPLATE_FNPTR_PLACEHOLDER_0(stackframe, PASS_MEM2REG_PARAMS, opaqueParams...);                                         \
        }                                                                                                                \
        else                                                                                                             \
        {                                                                                                                \
            INTERNAL_DEFINE_BOILERPLATE_FNPTR_PLACEHOLDER(1, void(*)(uintptr_t, MEM2REG_TYPES, OpaqueParams...) noexcept);   \
            BOILERPLATE_FNPTR_PLACEHOLDER_1(stackframe, PASS_MEM2REG_PARAMS, opaqueParams...);                                         \
        }                                                                                                                \
    }

    static auto metavars()
    {
        return CreateMetaVarList(
                    CreateEnumMetaVar<FIMem2RegOrdinal::X_END_OF_ENUM>("ordinal"),
                    CreateEnumMetaVar<FIMem2RegOrdinal::X_END_OF_ENUM>("ordinal2"),
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
    RegisterBoilerplate<FIMem2RegInt32CompCondBranchImpl>();
}
