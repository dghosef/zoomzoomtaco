#define POCHIVM_INSIDE_FASTINTERP_TPL_CPP

#include "fastinterp_tpl_common.hpp"
#include "fastinterp_tpl_mem2reg_util.hpp"

namespace PochiVM
{

struct FIMem2RegIncrement
{
    template<typename T,
             FIMem2RegOrdinal ordinal,
             FINumOpaqueIntegralParams numOIP,
             FINumOpaqueFloatingParams numOFP>
    static constexpr bool cond()
    {
        if (std::is_same<T, void>::value) { return false; }
        if (std::is_same<T, void*>::value) { return false; }
        if (std::is_same<T, bool>::value) { return false; }
        if (std::is_floating_point<T>::value) { return false; }
        if (static_cast<size_t>(ordinal) >= x_mem2reg_max_integral_vars) { return false; }
        if (FIOpaqueParamsHelper::CanPush(numOIP)) { return false; }
        if (FIOpaqueParamsHelper::CanPush(numOFP)) { return false; }
        return true;
    }

    template<typename T,
             FIMem2RegOrdinal ordinal,
             FINumOpaqueIntegralParams numOIP,
             FINumOpaqueFloatingParams numOFP,
             typename... OpaqueParams>
    static void f(uintptr_t stackframe,
                  GEN_MEM2REG_EXTRACT_DEFS(!std::is_floating_point<T>::value, static_cast<size_t>(ordinal), T),
                  OpaqueParams... opaqueParams,
                  [[maybe_unused]] T value) noexcept
    {
        // TODO: Restore numoip/numofp logic
        constexpr bool isInt = !std::is_floating_point<T>::value;
        constexpr size_t regOrd = static_cast<size_t>(ordinal);
        INSERT_MEM2REG_VALUE(isInt, regOrd) = EXTRACT_MEM2REG_VALUE(isInt, regOrd) + 1;
        DEFINE_BOILERPLATE_FNPTR_PLACEHOLDER_0(void(*)(uintptr_t, GEN_MEM2REG_EXTRACT_TYPES(isInt, regOrd, T), OpaqueParams...) noexcept);
        BOILERPLATE_FNPTR_PLACEHOLDER_0(stackframe, PASS_MEM2REG_PARAMS, opaqueParams...);
    }

    static auto metavars()
    {
        return CreateMetaVarList(
                    CreateTypeMetaVar("valueType"),
                    CreateEnumMetaVar<FIMem2RegOrdinal::X_END_OF_ENUM>("ordinal"),
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
    RegisterBoilerplate<FIMem2RegIncrement>();
}
