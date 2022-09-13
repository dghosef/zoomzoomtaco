
#define POCHIVM_INSIDE_FASTINTERP_TPL_CPP

#include "fastinterp_tpl_common.hpp"
#include "fastinterp_tpl_mem2reg_util.hpp"

/*
Supernodes for:
Everything we have so far
A[b+1]
A[i] = B[i] op C[i] op D[i] but not necessarily with variable indexing
a = (int)(b==c)
inline all taco_min calls
*/
namespace PochiVM
{

struct FIMem2RegDeref
{
    template<typename ArrType,
             FIMem2RegOrdinal ArrOrdinal,
             bool spillOutput,
             FINumOpaqueIntegralParams numOIP,
             FINumOpaqueFloatingParams numOFP>
    static constexpr bool cond()
    {
        if (!std::is_pointer_v<ArrType>) { return false; }
        using T = std::remove_pointer_t<ArrType>;
        if (std::is_same<T, void>::value) { return false; }
        if (std::is_pointer<T>::value && !std::is_same<T, void*>::value) { return false; }
        if (!spillOutput)
        {
            if (std::is_floating_point<T>::value)
            {
                if (!FIOpaqueParamsHelper::CanPush(numOFP)) { return false; }
                if (FIOpaqueParamsHelper::CanPush(numOIP)) { return false; }
            }
            else
            {
                if (FIOpaqueParamsHelper::CanPush(numOFP)) { return false; }
                if (!FIOpaqueParamsHelper::CanPush(numOIP)) { return false; }
            }
        }
        else
        {
            if (FIOpaqueParamsHelper::CanPush(numOFP)) { return false; }
            if (FIOpaqueParamsHelper::CanPush(numOIP)) { return false; }
        }
        if (static_cast<size_t>(ArrOrdinal) >= x_mem2reg_max_integral_vars) { return false; }
        return true;
    }

    template<typename ArrType,
             FIMem2RegOrdinal ArrOrdinal,
             bool spillOutput,
             FINumOpaqueIntegralParams numOIP,
             FINumOpaqueFloatingParams numOFP,
             typename... OpaqueParams>
    static void f(uintptr_t stackframe,
                  GEN_MEM2REG_EXTRACT_DEFS(true, static_cast<size_t>(ArrOrdinal), ArrType),
                  OpaqueParams... opaqueParams) noexcept
    {
        using T = std::remove_pointer_t<ArrType>;
        constexpr size_t arrOrd = static_cast<size_t>(ArrOrdinal);
        T result = EXTRACT_MEM2REG_VALUE(true /*isInt*/, arrOrd)[0];
        if constexpr(!spillOutput)
        {
            DEFINE_BOILERPLATE_FNPTR_PLACEHOLDER_0(void(*)(uintptr_t, GEN_MEM2REG_EXTRACT_TYPES(true, arrOrd, ArrType), OpaqueParams..., T) noexcept);
            BOILERPLATE_FNPTR_PLACEHOLDER_0(stackframe, PASS_MEM2REG_PARAMS, opaqueParams..., result);
        }
        else
        {
            DEFINE_INDEX_CONSTANT_PLACEHOLDER_0;
            *GetLocalVarAddress<T>(stackframe, CONSTANT_PLACEHOLDER_0) = result;

            DEFINE_BOILERPLATE_FNPTR_PLACEHOLDER_0(void(*)(uintptr_t, GEN_MEM2REG_EXTRACT_TYPES(true, arrOrd, ArrType), OpaqueParams...) noexcept);
            BOILERPLATE_FNPTR_PLACEHOLDER_0(stackframe, PASS_MEM2REG_PARAMS, opaqueParams...);
        }
    }

    static auto metavars()
    {
        return CreateMetaVarList(
                    CreateTypeMetaVar("arrType"),
                    CreateEnumMetaVar<FIMem2RegOrdinal::X_END_OF_ENUM>("ArrOrdinal"),
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
    RegisterBoilerplate<FIMem2RegDeref>();
}
