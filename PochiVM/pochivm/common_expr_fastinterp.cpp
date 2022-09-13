#include "fastinterp_ast_helper.hpp"
#include "arith_expr.h"

namespace PochiVM
{
bool isVar(AstNodeBase* base) {
    return base->GetAstNodeType() == AstNodeType::AstDereferenceVariableExpr || base->GetAstNodeType() == AstNodeType::AstRegisterCachedVariableExpr;
}

bool isArith(AstNodeBase* base) {
    return base->GetAstNodeType() == AstNodeType::AstArithmeticExpr;
}

bool isInReg(AstNodeBase* base) {
    return base->GetAstNodeType() == AstNodeType::AstRegisterCachedVariableExpr;
}

bool isOne(AstNodeBase* base) {
    return base->GetAstNodeType() == AstNodeType::AstLiteralExpr && assert_cast<AstLiteralExpr*>(base)->GetAsU64() == 1;
}

void AstDereferenceExpr::FastInterpSetupSpillLocation()
{
    if (m_operand->GetAstNodeType() == AstNodeType::AstPointerArithmeticExpr)
    {
        AstPointerArithmeticExpr* paExpr = assert_cast<AstPointerArithmeticExpr*>(m_operand);
        if (paExpr->m_isAddition)
        {
            AstFIOperandShape osc = AstFIOperandShape::TryMatch(paExpr->m_index);
            if (osc.MatchOK())
            {
                if (paExpr->m_base->GetAstNodeType() == AstNodeType::AstDereferenceVariableExpr)
                {
                    return;
                }
                else
                {
                    thread_pochiVMContext->m_fastInterpStackFrameManager->ReserveTemp(paExpr->m_base->GetTypeId());
                    paExpr->m_base->FastInterpSetupSpillLocation();
                    return;
                }
            }
        }
    }

    thread_pochiVMContext->m_fastInterpStackFrameManager->ReserveTemp(GetTypeId().AddPointer());
    m_operand->FastInterpSetupSpillLocation();
}

#ifdef TACO_SUPERNODES
static FIMem2RegOrdinal getRegOrdinal(AstNodeBase* base) {
    if(base->GetAstNodeType() == AstNodeType::AstRegisterCachedVariableExpr) {
        return static_cast<FIMem2RegOrdinal>(assert_cast<AstRegisterCachedVariableExpr*>(base)->m_regOrdinal);
    }
    return static_cast<FIMem2RegOrdinal>(0);
}
#endif
FastInterpSnippet WARN_UNUSED AstDereferenceExpr::PrepareForFastInterp(FISpillLocation spillLoc)
{
    if (m_operand->GetAstNodeType() == AstNodeType::AstPointerArithmeticExpr)
    {
        AstPointerArithmeticExpr* paExpr = assert_cast<AstPointerArithmeticExpr*>(m_operand);
        if (paExpr->m_isAddition)
        {
            // pattern match for the form arr[idx + 1]
            #ifdef TACO_SUPERNODES
            if(isArith(paExpr->m_index)) {
                AstArithmeticExpr* idxExpr = assert_cast<AstArithmeticExpr*>(paExpr->m_index);
                if(isVar(paExpr->m_base) && isVar(idxExpr->m_lhs) && isOne(idxExpr->m_rhs)) {
                    if(isInReg(paExpr->m_base) || isInReg(idxExpr->m_lhs)) {
                        FINumOpaqueIntegralParams numOIP = FIOpaqueParamsHelper::GetMaxOIP();
                        FINumOpaqueFloatingParams numOFP = FIOpaqueParamsHelper::GetMaxOFP();
                        if (spillLoc.IsNoSpill())
                        {
                            if (GetTypeId().IsFloatingPoint())
                            {
                                numOFP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillFloat();
                            }
                            else
                            {
                                numOIP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillIntegral();
                            }
                        }
                        FastInterpBoilerplateInstance* inst = thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
                                    FastInterpBoilerplateLibrary<FIMem2RegArrIdxPlus1>::SelectBoilerplateBluePrint(
                                        FastInterpTypeId(paExpr->m_base->GetTypeId()),
                                        getRegOrdinal(paExpr->m_base),
                                        getRegOrdinal(idxExpr->m_rhs),
                                        isInReg(paExpr->m_base),
                                        isInReg(paExpr->m_index),
                                        !spillLoc.IsNoSpill(),
                                        numOIP,
                                        numOFP
                                    ));
                        if(!isInReg(paExpr->m_base)) {
                            AstDereferenceVariableExpr* derefVarExpr = assert_cast<AstDereferenceVariableExpr*>(paExpr->m_base);
                            inst->PopulateConstantPlaceholder<uint64_t>(1, derefVarExpr->GetOperand()->GetFastInterpOffset());
                        } else if(!isInReg(paExpr->m_index)) {
                            AstDereferenceVariableExpr* derefVarExpr = assert_cast<AstDereferenceVariableExpr*>(paExpr->m_index);
                            inst->PopulateConstantPlaceholder<uint64_t>(1, derefVarExpr->GetOperand()->GetFastInterpOffset());
                        }
                        spillLoc.PopulatePlaceholderIfSpill(inst, 0);
                        return FastInterpSnippet { inst, inst };

                    }
                }
            }
            // pattern match for if either/both arr or idx is held in a register for form arr[idx]
            if(isVar(paExpr->m_base) && isVar(paExpr->m_index)) {
                if(isInReg(paExpr->m_base) || isInReg(paExpr->m_index)) {
                    FINumOpaqueIntegralParams numOIP = FIOpaqueParamsHelper::GetMaxOIP();
                    FINumOpaqueFloatingParams numOFP = FIOpaqueParamsHelper::GetMaxOFP();
                    if (spillLoc.IsNoSpill())
                    {
                        if (GetTypeId().IsFloatingPoint())
                        {
                            numOFP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillFloat();
                        }
                        else
                        {
                            numOIP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillIntegral();
                        }
                    }
                    FastInterpBoilerplateInstance* inst = thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
                                FastInterpBoilerplateLibrary<FIMem2RegArrIdx>::SelectBoilerplateBluePrint(
                                    FastInterpTypeId(paExpr->m_base->GetTypeId()),
                                    getRegOrdinal(paExpr->m_base),
                                    getRegOrdinal(paExpr->m_index),
                                    isInReg(paExpr->m_base),
                                    isInReg(paExpr->m_index),
                                    !spillLoc.IsNoSpill(),
                                    numOIP,
                                    numOFP
                                ));
                    if(!isInReg(paExpr->m_base)) {
                        AstDereferenceVariableExpr* derefVarExpr = assert_cast<AstDereferenceVariableExpr*>(paExpr->m_base);
                        inst->PopulateConstantPlaceholder<uint64_t>(1, derefVarExpr->GetOperand()->GetFastInterpOffset());
                    } else if(!isInReg(paExpr->m_index)) {
                        AstDereferenceVariableExpr* derefVarExpr = assert_cast<AstDereferenceVariableExpr*>(paExpr->m_index);
                        inst->PopulateConstantPlaceholder<uint64_t>(1, derefVarExpr->GetOperand()->GetFastInterpOffset());
                    }
                    spillLoc.PopulatePlaceholderIfSpill(inst, 0);
                    return FastInterpSnippet { inst, inst };

                }
            }
            // pattern match for arr[idx] where arr is held in a register
            if(paExpr->m_base->GetAstNodeType() == AstNodeType::AstRegisterCachedVariableExpr) {
                // pattern match for when idx is of the form arr2[idx2] and arr2 and idx2 are both in registers
                if(paExpr->m_index->GetAstNodeType() == AstNodeType::AstDereferenceExpr) {
                    auto outer_idx = assert_cast<AstDereferenceExpr*>(paExpr->m_index);
                    if(outer_idx->m_operand->GetAstNodeType() == AstNodeType::AstPointerArithmeticExpr) {
                        auto inner_paExpr = assert_cast<AstPointerArithmeticExpr*>(outer_idx->m_operand);
                        if(inner_paExpr->m_isAddition) {
                            if(inner_paExpr->m_base->GetAstNodeType() == AstNodeType::AstRegisterCachedVariableExpr && inner_paExpr->m_index->GetAstNodeType() == AstNodeType::AstRegisterCachedVariableExpr) {
                                AstRegisterCachedVariableExpr* outer_arr = assert_cast<AstRegisterCachedVariableExpr*>(paExpr->m_base);
                                AstRegisterCachedVariableExpr* inner_arr = assert_cast<AstRegisterCachedVariableExpr*>(inner_paExpr->m_base);
                                AstRegisterCachedVariableExpr* inner_idx = assert_cast<AstRegisterCachedVariableExpr*>(inner_paExpr->m_index);
                                bool isValidType = true;
                                if(!outer_arr->GetTypeId().IsType<double*>()) { isValidType = false; }
                                if(!inner_arr->GetTypeId().RemovePointer().IsPrimitiveType() || inner_arr->GetTypeId().RemovePointer().Size() != 4) { isValidType = false; }
                                if(!inner_idx->GetTypeId().IsPrimitiveIntType()) { isValidType = false; }
                                if(isValidType) {
                                    FINumOpaqueIntegralParams numOIP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillIntegral();
                                    FINumOpaqueFloatingParams numOFP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillFloat();
                                    if (!spillLoc.IsNoSpill())
                                    {
                                        numOIP = FIOpaqueParamsHelper::GetMaxOIP();
                                        numOFP = FIOpaqueParamsHelper::GetMaxOFP();
                                    }
                                    else if (GetTypeId().IsFloatingPoint())
                                    {
                                        numOIP = FIOpaqueParamsHelper::GetMaxOIP();
                                    }
                                    else
                                    {
                                        numOFP = FIOpaqueParamsHelper::GetMaxOFP();
                                    }
                                    FastInterpBoilerplateInstance* inst = thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
                                                FastInterpBoilerplateLibrary<FIMem2RegInt32ArrArrVarIdx>::SelectBoilerplateBluePrint(
                                                    static_cast<FIMem2RegOrdinal>(outer_arr->m_regOrdinal),
                                                    static_cast<FIMem2RegOrdinal>(inner_arr->m_regOrdinal),
                                                    static_cast<FIMem2RegOrdinal>(inner_idx->m_regOrdinal),
                                                    !spillLoc.IsNoSpill(),
                                                    numOIP,
                                                    numOFP));
                                    spillLoc.PopulatePlaceholderIfSpill(inst, 0);
                                    return FastInterpSnippet { inst, inst };
                                }
                            }
                        }
                    }
                }
                #if 0
                // pattern match for when idx is also in a register
                if(paExpr->m_index->GetAstNodeType() == AstNodeType::AstRegisterCachedVariableExpr) {
                    auto var = assert_cast<AstRegisterCachedVariableExpr *>(paExpr->m_base);
                    auto index = assert_cast<AstRegisterCachedVariableExpr *>(paExpr->m_index);
                    // If the array is an 32 bit int*
                    if(var->GetTypeId().RemovePointer().IsPrimitiveIntType() && var->GetTypeId().RemovePointer().Size() == 4) {
                        // TODO: Fix numoip/numofp stuff
                        FINumOpaqueIntegralParams numOIP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillIntegral();
                        FINumOpaqueFloatingParams numOFP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillFloat();
                        if (!spillLoc.IsNoSpill())
                        {
                            numOIP = FIOpaqueParamsHelper::GetMaxOIP();
                            numOFP = FIOpaqueParamsHelper::GetMaxOFP();
                        }
                        else if (GetTypeId().IsFloatingPoint())
                        {
                            numOIP = FIOpaqueParamsHelper::GetMaxOIP();
                        }
                        else
                        {
                            numOFP = FIOpaqueParamsHelper::GetMaxOFP();
                        }
                        FastInterpBoilerplateInstance* inst = thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
                                    FastInterpBoilerplateLibrary<FIMem2RegInt32ArrVarIdx>::SelectBoilerplateBluePrint(
                                        static_cast<FIMem2RegOrdinal>(var->m_regOrdinal),
                                        static_cast<FIMem2RegOrdinal>(index->m_regOrdinal),
                                        !spillLoc.IsNoSpill(),
                                        numOIP,
                                        numOFP));
                        spillLoc.PopulatePlaceholderIfSpill(inst, 0);
            std::cout << paExpr->m_base->GetAstNodeType().ToString() << std::endl;
            std::cout << paExpr->m_index->GetAstNodeType().ToString() << std::endl;
                        return FastInterpSnippet { inst, inst };
                    }
                    // If the arr is a double*
                    if(var->GetTypeId().RemovePointer().IsDouble()) {
                        // TODO: Fix numoip/numofp stuff
                        FINumOpaqueIntegralParams numOIP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillIntegral();
                        FINumOpaqueFloatingParams numOFP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillFloat();
                        if (!spillLoc.IsNoSpill())
                        {
                            numOIP = FIOpaqueParamsHelper::GetMaxOIP();
                            numOFP = FIOpaqueParamsHelper::GetMaxOFP();
                        }
                        else if (GetTypeId().IsFloatingPoint())
                        {
                            numOIP = FIOpaqueParamsHelper::GetMaxOIP();
                        }
                        else
                        {
                            numOFP = FIOpaqueParamsHelper::GetMaxOFP();
                        }
                        FastInterpBoilerplateInstance* inst = thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
                                    FastInterpBoilerplateLibrary<FIMem2RegDoubleArrVarIdx>::SelectBoilerplateBluePrint(
                                        static_cast<FIMem2RegOrdinal>(var->m_regOrdinal),
                                        static_cast<FIMem2RegOrdinal>(index->m_regOrdinal),
                                        !spillLoc.IsNoSpill(),
                                        numOIP,
                                        numOFP));
                        spillLoc.PopulatePlaceholderIfSpill(inst, 0);
            std::cout << paExpr->m_base->GetAstNodeType().ToString() << std::endl;
            std::cout << paExpr->m_index->GetAstNodeType().ToString() << std::endl;
                        return FastInterpSnippet { inst, inst };
                    }
                }
                #endif
                // pattern match for arr[0]
                if(paExpr->m_index->GetAstNodeType() == AstNodeType::AstLiteralExpr) {
                    auto var = assert_cast<AstRegisterCachedVariableExpr *>(paExpr->m_base);
                    auto index = assert_cast<AstLiteralExpr *>(paExpr->m_index);
                    if(index->GetAsU64() == 0) {
                        FINumOpaqueIntegralParams numOIP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillIntegral();
                        FINumOpaqueFloatingParams numOFP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillFloat();
                        if (!spillLoc.IsNoSpill())
                        {
                            numOIP = FIOpaqueParamsHelper::GetMaxOIP();
                            numOFP = FIOpaqueParamsHelper::GetMaxOFP();
                        }
                        else if (GetTypeId().IsFloatingPoint())
                        {
                            numOIP = FIOpaqueParamsHelper::GetMaxOIP();
                        }
                        else
                        {
                            numOFP = FIOpaqueParamsHelper::GetMaxOFP();
                        }
                        FastInterpBoilerplateInstance* inst = thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
                                    FastInterpBoilerplateLibrary<FIMem2RegDeref>::SelectBoilerplateBluePrint(
                                        FastInterpTypeId(var->m_variable->GetTypeId().RemovePointer()),
                                        static_cast<FIMem2RegOrdinal>(var->m_regOrdinal),
                                        !spillLoc.IsNoSpill(),
                                        numOIP,
                                        numOFP));
                        spillLoc.PopulatePlaceholderIfSpill(inst, 0);
                        return FastInterpSnippet { inst, inst };
                    }
                }
            }
            #endif
            AstFIOperandShape osc = AstFIOperandShape::TryMatch(paExpr->m_index);
            if (osc.MatchOK())
            {
                if (paExpr->m_base->GetAstNodeType() == AstNodeType::AstDereferenceVariableExpr)
                {
                    FINumOpaqueIntegralParams numOIP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillIntegral();
                    FINumOpaqueFloatingParams numOFP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillFloat();
                    if (!spillLoc.IsNoSpill())
                    {
                        numOIP = FIOpaqueParamsHelper::GetMaxOIP();
                        numOFP = FIOpaqueParamsHelper::GetMaxOFP();
                    }
                    else if (GetTypeId().IsFloatingPoint())
                    {
                        numOIP = FIOpaqueParamsHelper::GetMaxOIP();
                    }
                    else
                    {
                        numOFP = FIOpaqueParamsHelper::GetMaxOFP();
                    }
                    AstDereferenceVariableExpr* derefVarExpr = assert_cast<AstDereferenceVariableExpr*>(paExpr->m_base);
                    FastInterpBoilerplateInstance* inst = thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
                                FastInterpBoilerplateLibrary<FIInlinedDereferenceImpl>::SelectBoilerplateBluePrint(
                                    GetTypeId().GetOneLevelPtrFastInterpTypeId(),
                                    paExpr->m_index->GetTypeId().GetOneLevelPtrFastInterpTypeId(),
                                    osc.m_indexType,
                                    osc.m_kind,
                                    !spillLoc.IsNoSpill(),
                                    numOIP,
                                    numOFP));
                    spillLoc.PopulatePlaceholderIfSpill(inst, 0);
                    inst->PopulateConstantPlaceholder<uint64_t>(1, derefVarExpr->GetOperand()->GetFastInterpOffset());
                    osc.PopulatePlaceholder(inst, 2, 3);
                    return FastInterpSnippet { inst, inst };
                }
                else
                {
                    TestAssert(thread_pochiVMContext->m_fastInterpStackFrameManager->CanReserveWithoutSpill(paExpr->m_base->GetTypeId()));
                    FastInterpSnippet snippet = paExpr->m_base->PrepareForFastInterp(x_FINoSpill);

                    FINumOpaqueIntegralParams numOIP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillIntegral();
                    FINumOpaqueFloatingParams numOFP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillFloat();
                    if (!spillLoc.IsNoSpill() || !GetTypeId().IsFloatingPoint())
                    {
                        numOFP = FIOpaqueParamsHelper::GetMaxOFP();
                    }
                    FastInterpBoilerplateInstance* inst = thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
                                FastInterpBoilerplateLibrary<FIPartialInlineRhsDereferenceImpl>::SelectBoilerplateBluePrint(
                                    GetTypeId().GetOneLevelPtrFastInterpTypeId(),
                                    paExpr->m_index->GetTypeId().GetOneLevelPtrFastInterpTypeId(),
                                    osc.m_indexType,
                                    osc.m_kind,
                                    !spillLoc.IsNoSpill(),
                                    numOIP,
                                    numOFP));
                    spillLoc.PopulatePlaceholderIfSpill(inst, 0);
                    osc.PopulatePlaceholder(inst, 1, 2);
                    return snippet.AddContinuation(inst);
                }
            }
        }
    }

    {
        TestAssert(thread_pochiVMContext->m_fastInterpStackFrameManager->CanReserveWithoutSpill(GetTypeId().AddPointer()));
        FastInterpSnippet snippet = m_operand->PrepareForFastInterp(x_FINoSpill);
        FINumOpaqueFloatingParams numOFP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillFloat();
        if (!spillLoc.IsNoSpill() || !GetTypeId().IsFloatingPoint())
        {
            numOFP = FIOpaqueParamsHelper::GetMaxOFP();
        }
        FastInterpBoilerplateInstance* inst = thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
                    FastInterpBoilerplateLibrary<FIPartialInlineRhsDereferenceImpl>::SelectBoilerplateBluePrint(
                        GetTypeId().GetOneLevelPtrFastInterpTypeId(),
                        TypeId::Get<uint64_t>().GetDefaultFastInterpTypeId(),
                        TypeId::Get<int32_t>().GetDefaultFastInterpTypeId(),
                        FIOperandShapeCategory::ZERO,
                        !spillLoc.IsNoSpill(),
                        thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillIntegral(),
                        numOFP));
        spillLoc.PopulatePlaceholderIfSpill(inst, 0);
        return snippet.AddContinuation(inst);
    }
}

FastInterpSnippet WARN_UNUSED AstLiteralExpr::PrepareForFastInterp(FISpillLocation spillLoc)
{
    bool isAllBitsZero = IsAllBitsZero();
    FINumOpaqueIntegralParams numOIP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillIntegral();
    FINumOpaqueFloatingParams numOFP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillFloat();
    if (!spillLoc.IsNoSpill())
    {
        numOIP = FIOpaqueParamsHelper::GetMaxOIP();
        numOFP = FIOpaqueParamsHelper::GetMaxOFP();
    }
    else if (GetTypeId().IsFloatingPoint())
    {
        numOIP = FIOpaqueParamsHelper::GetMaxOIP();
    }
    else
    {
        numOFP = FIOpaqueParamsHelper::GetMaxOFP();
    }

    FastInterpBoilerplateInstance* inst;
    if (IsTypeIdConstantValidForSmallCodeModel(GetTypeId()))
    {
        inst = thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
                    FastInterpBoilerplateLibrary<FILiteralMcSmallImpl>::SelectBoilerplateBluePrint(
                        GetTypeId().GetOneLevelPtrFastInterpTypeId(),
                        isAllBitsZero,
                        !spillLoc.IsNoSpill(),
                        numOIP,
                        numOFP));
    }
    else
    {
        inst = thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
                    FastInterpBoilerplateLibrary<FILiteralMcMediumImpl>::SelectBoilerplateBluePrint(
                        GetTypeId().GetOneLevelPtrFastInterpTypeId(),
                        isAllBitsZero,
                        !spillLoc.IsNoSpill(),
                        numOIP,
                        numOFP));
    }
    spillLoc.PopulatePlaceholderIfSpill(inst, 0);
    if (!isAllBitsZero)
    {
        inst->PopulateConstantPlaceholder<uint64_t>(1, m_as_uint64_t);
    }
    return FastInterpSnippet { inst, inst };
}

void AstAssignExpr::FastInterpSetupSpillLocation()
{
    TestAssert(m_fiInlineShape == FIShape::INVALID);
    Auto(TestAssert(m_fiInlineShape != FIShape::INVALID));
    // Special case: LHS is a mem2reg variable
    //
    if (m_isLhsMem2RegVar)
    {
        #ifndef TACO_SUPERNODES

        thread_pochiVMContext->m_fastInterpStackFrameManager->ReserveTemp(m_src->GetTypeId());
        m_src->FastInterpSetupSpillLocation();
        m_fiInlineShape = FIShape::INLINE_LHS;

        #else

        if(!m_isIncrement) {
            // only need to do this if we're incrementiing
            thread_pochiVMContext->m_fastInterpStackFrameManager->ReserveTemp(m_src->GetTypeId());
            m_src->FastInterpSetupSpillLocation();
            m_fiInlineShape = FIShape::INLINE_LHS;
        }
        else {
            m_fiInlineShape = FIShape::INLINE_LHS;
        }

        #endif
        return;
    }


    // Case var = osc op osc
    // FIFullyInlineAssignArithExprImpl
    //
    {
        if (m_dst->GetAstNodeType() == AstNodeType::AstVariable && m_src->GetAstNodeType() == AstNodeType::AstArithmeticExpr &&
            (!m_src->GetTypeId().IsPointerType() && !m_src->GetTypeId().IsBool()))
        {
            AstArithmeticExpr* expr = assert_cast<AstArithmeticExpr*>(m_src);
            AstFIOperandShape lhs = AstFIOperandShape::TryMatch(expr->m_lhs);
            if (lhs.MatchOK())
            {
                AstFIOperandShape rhs = AstFIOperandShape::TryMatch(expr->m_rhs);
                if (rhs.MatchOK())
                {
                    m_fiInlineShape = FIShape::INLINE_ARITH;
                    return;
                }
            }
        }
    }

    // Other cases
    //
    {
        AstFIOperandShape lhs = AstFIOperandShape::TryMatchAddress(m_dst);
        AstFIOperandShape rhs = AstFIOperandShape::TryMatch(m_src);

        if (lhs.MatchOK() && rhs.MatchOK())
        {
            // FIFullyInlineAssignImpl
            //
            m_fiInlineShape = FIShape::INLINE_BOTH;
            return;
        }
        else if (lhs.MatchOK())
        {
            // FIPartialInlineLhsAssignImpl
            //
            thread_pochiVMContext->m_fastInterpStackFrameManager->ReserveTemp(m_src->GetTypeId());
            m_src->FastInterpSetupSpillLocation();
            m_fiInlineShape = FIShape::INLINE_LHS;
            return;
        }
        else if (rhs.MatchOK())
        {
            // FIPartialInlineRhsAssignImpl
            //
            thread_pochiVMContext->m_fastInterpStackFrameManager->ReserveTemp(m_dst->GetTypeId());
            m_dst->FastInterpSetupSpillLocation();
            m_fiInlineShape = FIShape::INLINE_RHS;
            return;
        }
        else
        {
            // FIOutlinedAssignImpl
            //
            thread_pochiVMContext->m_fastInterpStackFrameManager->PushTemp(m_dst->GetTypeId());
            thread_pochiVMContext->m_fastInterpStackFrameManager->ReserveTemp(m_src->GetTypeId());
            m_src->FastInterpSetupSpillLocation();
            FISpillLocation lhsSpillLoc = thread_pochiVMContext->m_fastInterpStackFrameManager->PopTemp(m_dst->GetTypeId());
            m_fiIsLhsSpill = !lhsSpillLoc.IsNoSpill();
            m_dst->FastInterpSetupSpillLocation();
            m_fiInlineShape = FIShape::OUTLINE;
            return;
        }
    }
}

FastInterpSnippet WARN_UNUSED AstAssignExpr::PrepareForFastInterp(FISpillLocation TESTBUILD_ONLY(spillLoc))
{
    TestAssert(spillLoc.IsNoSpill());
    TestAssert(m_fiInlineShape != FIShape::INVALID);
   if (m_isLhsMem2RegVar)
    {
        TestAssert(m_fiInlineShape == FIShape::INLINE_LHS);

        FINumOpaqueIntegralParams numOIP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillIntegral();
        FINumOpaqueFloatingParams numOFP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillFloat();
        if (m_src->GetTypeId().IsFloatingPoint())
        {
            numOIP = FIOpaqueParamsHelper::GetMaxOIP();
        }
        else
        {
            numOFP = FIOpaqueParamsHelper::GetMaxOFP();
        }
        FastInterpBoilerplateInstance* inst = nullptr;
        #ifdef TACO_SUPERNODES
            if(m_isIncrement) {
                inst = thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
                        FastInterpBoilerplateLibrary<FIMem2RegIncrement>::SelectBoilerplateBluePrint(
                            FastInterpTypeId(m_dst->GetTypeId().RemovePointer()),
                            static_cast<FIMem2RegOrdinal>(m_mem2regDst->m_regOrdinal),
                            FIOpaqueParamsHelper::GetMaxOIP(),
                            FIOpaqueParamsHelper::GetMaxOFP()));
                return FastInterpSnippet {inst, inst};
            }
        #endif
        {
            TestAssert(thread_pochiVMContext->m_fastInterpStackFrameManager->CanReserveWithoutSpill(m_src->GetTypeId()));
            FastInterpSnippet snippet = m_src->PrepareForFastInterp(x_FINoSpill);
            inst = thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
                    FastInterpBoilerplateLibrary<FIMem2RegInsertValue>::SelectBoilerplateBluePrint(
                        m_src->GetTypeId().GetOneLevelPtrFastInterpTypeId(),
                        false /*fromStack*/,
                        static_cast<FIMem2RegOrdinal>(m_mem2regDst->m_regOrdinal),
                        numOIP,
                        numOFP));
            return snippet.AddContinuation(inst);
        }
    }
    if (m_fiInlineShape == FIShape::INLINE_ARITH)
    {
        // Case var = osc op osc
        // FIFullyInlineAssignArithExprImpl
        //
        TestAssert(m_dst->GetAstNodeType() == AstNodeType::AstVariable && m_src->GetAstNodeType() == AstNodeType::AstArithmeticExpr &&
                   (!m_src->GetTypeId().IsPointerType() && !m_src->GetTypeId().IsBool()));
        AstArithmeticExpr* expr = assert_cast<AstArithmeticExpr*>(m_src);
        AstFIOperandShape lhs = AstFIOperandShape::TryMatch(expr->m_lhs);
        TestAssert(lhs.MatchOK());
        AstFIOperandShape rhs = AstFIOperandShape::TryMatch(expr->m_rhs);
        TestAssert(rhs.MatchOK());
        AstVariable* var = assert_cast<AstVariable*>(m_dst);
        FastInterpBoilerplateInstance* inst = thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
                    FastInterpBoilerplateLibrary<FIFullyInlineAssignArithExprImpl>::SelectBoilerplateBluePrint(
                        m_src->GetTypeId().GetOneLevelPtrFastInterpTypeId(),
                        lhs.m_indexType,
                        rhs.m_indexType,
                        lhs.m_kind,
                        rhs.m_kind,
                        FIOpaqueParamsHelper::GetMaxOIP(),
                        FIOpaqueParamsHelper::GetMaxOFP(),
                        expr->m_op));
        inst->PopulateConstantPlaceholder<uint64_t>(0, var->GetFastInterpOffset());
        lhs.PopulatePlaceholder(inst, 1, 2);
        rhs.PopulatePlaceholder(inst, 3, 4);
        return FastInterpSnippet {
            inst, inst
        };
    }
    else if (m_fiInlineShape == FIShape::INLINE_BOTH)
    {
        AstFIOperandShape lhs = AstFIOperandShape::TryMatchAddress(m_dst);
        TestAssert(lhs.MatchOK());
        AstFIOperandShape rhs = AstFIOperandShape::TryMatch(m_src);
        TestAssert(rhs.MatchOK());

        // FIFullyInlineAssignImpl
        //
        FastInterpBoilerplateInstance* inst = thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
                    FastInterpBoilerplateLibrary<FIFullyInlineAssignImpl>::SelectBoilerplateBluePrint(
                        m_src->GetTypeId().GetOneLevelPtrFastInterpTypeId(),
                        lhs.m_indexType,
                        rhs.m_indexType,
                        lhs.m_kind,
                        rhs.m_kind,
                        FIOpaqueParamsHelper::GetMaxOIP(),
                        FIOpaqueParamsHelper::GetMaxOFP()));
        lhs.PopulatePlaceholder(inst, 0, 1);
        rhs.PopulatePlaceholder(inst, 2, 3);
        return FastInterpSnippet {
            inst, inst
        };
    }
    else if (m_fiInlineShape == FIShape::INLINE_LHS)
    {
        // FIPartialInlineLhsAssignImpl
        //
        AstFIOperandShape lhs = AstFIOperandShape::TryMatchAddress(m_dst);
        TestAssert(lhs.MatchOK());

        TestAssert(thread_pochiVMContext->m_fastInterpStackFrameManager->CanReserveWithoutSpill(m_src->GetTypeId()));
        FastInterpSnippet snippet = m_src->PrepareForFastInterp(x_FINoSpill);

        FINumOpaqueIntegralParams numOIP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillIntegral();
        FINumOpaqueFloatingParams numOFP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillFloat();
        if (m_src->GetTypeId().IsFloatingPoint())
        {
            numOIP = FIOpaqueParamsHelper::GetMaxOIP();
        }
        else
        {
            numOFP = FIOpaqueParamsHelper::GetMaxOFP();
        }
        FastInterpBoilerplateInstance* inst = thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
                    FastInterpBoilerplateLibrary<FIPartialInlineLhsAssignImpl>::SelectBoilerplateBluePrint(
                        m_src->GetTypeId().GetOneLevelPtrFastInterpTypeId(),
                        lhs.m_indexType,
                        lhs.m_kind,
                        numOIP,
                        numOFP));
        lhs.PopulatePlaceholder(inst, 0, 1);
        return snippet.AddContinuation(inst);
    }
    else if (m_fiInlineShape == FIShape::INLINE_RHS)
    {
        // FIPartialInlineRhsAssignImpl
        //
        AstFIOperandShape rhs = AstFIOperandShape::TryMatch(m_src);
        TestAssert(rhs.MatchOK());
        TestAssert(thread_pochiVMContext->m_fastInterpStackFrameManager->CanReserveWithoutSpill(m_dst->GetTypeId()));

        FastInterpSnippet snippet = m_dst->PrepareForFastInterp(x_FINoSpill);

        FastInterpBoilerplateInstance* inst = thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
                    FastInterpBoilerplateLibrary<FIPartialInlineRhsAssignImpl>::SelectBoilerplateBluePrint(
                        m_src->GetTypeId().GetOneLevelPtrFastInterpTypeId(),
                        rhs.m_indexType,
                        rhs.m_kind,
                        thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillIntegral(),
                        FIOpaqueParamsHelper::GetMaxOFP()));
        rhs.PopulatePlaceholder(inst, 0, 1);
        return snippet.AddContinuation(inst);
    }
    else
    {
        // FIOutlinedAssignImpl
        //
        TestAssert(m_fiInlineShape == FIShape::OUTLINE);
        thread_pochiVMContext->m_fastInterpStackFrameManager->PushTemp(m_dst->GetTypeId(), m_fiIsLhsSpill);
        TestAssert(thread_pochiVMContext->m_fastInterpStackFrameManager->CanReserveWithoutSpill(m_src->GetTypeId()));
        FastInterpSnippet rhsSnippet = m_src->PrepareForFastInterp(x_FINoSpill);
        FISpillLocation lhsSpillLoc = thread_pochiVMContext->m_fastInterpStackFrameManager->PopTemp(m_dst->GetTypeId());
        TestAssertIff(m_fiIsLhsSpill, !lhsSpillLoc.IsNoSpill());
        FastInterpSnippet lhsSnippet = m_dst->PrepareForFastInterp(lhsSpillLoc);

        FINumOpaqueFloatingParams numOFP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillFloat();
        if (!m_src->GetTypeId().IsFloatingPoint())
        {
            numOFP = FIOpaqueParamsHelper::GetMaxOFP();
        }
        FastInterpBoilerplateInstance* inst = thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
                    FastInterpBoilerplateLibrary<FIOutlinedAssignImpl>::SelectBoilerplateBluePrint(
                        m_src->GetTypeId().GetOneLevelPtrFastInterpTypeId(),
                        lhsSpillLoc.IsNoSpill(),
                        thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillIntegral(),
                        numOFP));
        lhsSpillLoc.PopulatePlaceholderIfSpill(inst, 0);
        return lhsSnippet.AddContinuation(rhsSnippet).AddContinuation(inst);
    }
}

void AstRvalueToConstPrimitiveRefExpr::FastInterpSetupSpillLocation()
{
    thread_pochiVMContext->m_fastInterpStackFrameManager->ReserveTemp(m_operand->GetTypeId());
    m_operand->FastInterpSetupSpillLocation();
}

FastInterpSnippet WARN_UNUSED AstExceptionAddressPlaceholder::PrepareForFastInterp(FISpillLocation spillLoc)
{
    TestAssert(m_fastInterpStackOffsetSet);
    FINumOpaqueIntegralParams numOIP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillIntegral();
    FINumOpaqueFloatingParams numOFP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillFloat();
    FastInterpBoilerplateInstance* inst = thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
                FastInterpBoilerplateLibrary<FIVariableImpl>::SelectBoilerplateBluePrint(
                    GetTypeId().GetDefaultFastInterpTypeId(),
                    !spillLoc.IsNoSpill(),
                    numOIP,
                    numOFP));
    spillLoc.PopulatePlaceholderIfSpill(inst, 0);
    inst->PopulateConstantPlaceholder<uint64_t>(1, m_fastInterpStackOffset);
    return FastInterpSnippet {
        inst, inst
    };
}

void AstPointerArithmeticExpr::FastInterpSetupSpillLocation()
{
    if (m_base->GetAstNodeType() == AstNodeType::AstDereferenceVariableExpr)
    {
        thread_pochiVMContext->m_fastInterpStackFrameManager->ReserveTemp(m_index->GetTypeId());
        m_index->FastInterpSetupSpillLocation();
    }
    else
    {
        thread_pochiVMContext->m_fastInterpStackFrameManager->PushTemp(m_base->GetTypeId());
        thread_pochiVMContext->m_fastInterpStackFrameManager->ReserveTemp(m_index->GetTypeId());
        m_index->FastInterpSetupSpillLocation();
        FISpillLocation spillLoc = thread_pochiVMContext->m_fastInterpStackFrameManager->PopTemp(GetTypeId());
        m_fiIsBaseSpill = !spillLoc.IsNoSpill();
        m_base->FastInterpSetupSpillLocation();
    }
}

FastInterpSnippet WARN_UNUSED AstPointerArithmeticExpr::PrepareForFastInterp(FISpillLocation spillLoc)
{
    size_t objectSize = m_base->GetTypeId().RemovePointer().Size();
    TestAssert(objectSize < 1ULL << 31);
    FIPowerOfTwoObjectSize po2Size;
    if (math::is_power_of_2(static_cast<int>(objectSize)))
    {
        int log2 = __builtin_ctz(static_cast<unsigned int>(objectSize));
        TestAssert((1ULL << log2) == objectSize);
        if (log2 >= static_cast<int>(FIPowerOfTwoObjectSize::NOT_POWER_OF_TWO))
        {
            po2Size = FIPowerOfTwoObjectSize::NOT_POWER_OF_TWO;
        }
        else
        {
            po2Size = static_cast<FIPowerOfTwoObjectSize>(log2);
        }
    }
    else
    {
        po2Size = FIPowerOfTwoObjectSize::NOT_POWER_OF_TWO;
    }
    #ifdef TACO_SUPERNODES
    if(isVar(m_base) && isVar(m_index)) {
        if((isInReg(m_base) || isInReg(m_index))) {

            FINumOpaqueIntegralParams numOIP = FIOpaqueParamsHelper::GetMaxOIP();
            FINumOpaqueFloatingParams numOFP = FIOpaqueParamsHelper::GetMaxOFP();
            if (spillLoc.IsNoSpill())
            {
                if (GetTypeId().IsFloatingPoint())
                {
                    numOFP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillFloat();
                }
                else
                {
                    numOIP = thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillIntegral();
                }
            }
            FastInterpBoilerplateInstance* inst = thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
                        FastInterpBoilerplateLibrary<FIMem2RegPtrAdd>::SelectBoilerplateBluePrint(
                            FastInterpTypeId(m_base->GetTypeId()),
                            getRegOrdinal(m_base),
                            getRegOrdinal(m_index),
                            isInReg(m_base),
                            isInReg(m_index),
                            !spillLoc.IsNoSpill(),
                            numOIP,
                            numOFP
                        ));
            if(!isInReg(m_base)) {
                AstDereferenceVariableExpr* derefVarExpr = assert_cast<AstDereferenceVariableExpr*>(m_base);
                inst->PopulateConstantPlaceholder<uint64_t>(1, derefVarExpr->GetOperand()->GetFastInterpOffset());
            } else if(!isInReg(m_index)) {
                AstDereferenceVariableExpr* derefVarExpr = assert_cast<AstDereferenceVariableExpr*>(m_index);
                inst->PopulateConstantPlaceholder<uint64_t>(1, derefVarExpr->GetOperand()->GetFastInterpOffset());
            }
            spillLoc.PopulatePlaceholderIfSpill(inst, 0);
            return FastInterpSnippet { inst, inst };
        }
    }
    #endif

    if (m_base->GetAstNodeType() == AstNodeType::AstDereferenceVariableExpr)
    {
        // Case 1: inline LHS
        //
        TestAssert(thread_pochiVMContext->m_fastInterpStackFrameManager->CanReserveWithoutSpill(m_index->GetTypeId()));
        FastInterpSnippet snippet = m_index->PrepareForFastInterp(x_FINoSpill);
        FastInterpBoilerplateInstance* inst = thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
                    FastInterpBoilerplateLibrary<FIPartialInlineLhsPointerArithmeticImpl>::SelectBoilerplateBluePrint(
                        m_index->GetTypeId().GetDefaultFastInterpTypeId() /*indexType*/,
                        m_isAddition ? AstArithmeticExprType::ADD : AstArithmeticExprType::SUB,
                        !spillLoc.IsNoSpill(),
                        thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillIntegral(),
                        FIOpaqueParamsHelper::GetMaxOFP(),
                        po2Size));
        spillLoc.PopulatePlaceholderIfSpill(inst, 0);

        AstDereferenceVariableExpr* expr = assert_cast<AstDereferenceVariableExpr*>(m_base);
        inst->PopulateConstantPlaceholder<uint64_t>(1, expr->GetOperand()->GetFastInterpOffset());

        if (po2Size == FIPowerOfTwoObjectSize::NOT_POWER_OF_TWO)
        {
            inst->PopulateConstantPlaceholder<uint64_t>(2, objectSize);
        }
        return snippet.AddContinuation(inst);
    }
    else
    {
        // Case 2: outlined
        //
        thread_pochiVMContext->m_fastInterpStackFrameManager->PushTemp(m_base->GetTypeId(), m_fiIsBaseSpill);
        TestAssert(thread_pochiVMContext->m_fastInterpStackFrameManager->CanReserveWithoutSpill(m_index->GetTypeId()));
        FastInterpSnippet rhsSnippet = m_index->PrepareForFastInterp(x_FINoSpill);
        FISpillLocation lhsSpillLoc = thread_pochiVMContext->m_fastInterpStackFrameManager->PopTemp(m_base->GetTypeId());
        TestAssertIff(m_fiIsBaseSpill, !lhsSpillLoc.IsNoSpill());
        FastInterpSnippet lhsSnippet = m_base->PrepareForFastInterp(lhsSpillLoc);

        if (lhsSpillLoc.IsNoSpill())
        {
            FastInterpBoilerplateInstance* inst = thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
                        FastInterpBoilerplateLibrary<FIOutlinedPointerArithmeticImpl>::SelectBoilerplateBluePrint(
                            m_index->GetTypeId().GetDefaultFastInterpTypeId(),
                            m_isAddition ? AstArithmeticExprType::ADD : AstArithmeticExprType::SUB,
                            !spillLoc.IsNoSpill(),
                            thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillIntegral(),
                            FIOpaqueParamsHelper::GetMaxOFP(),
                            po2Size));
            spillLoc.PopulatePlaceholderIfSpill(inst, 0);
            if (po2Size == FIPowerOfTwoObjectSize::NOT_POWER_OF_TWO)
            {
                inst->PopulateConstantPlaceholder<uint64_t>(1, objectSize);
            }
            return lhsSnippet.AddContinuation(rhsSnippet).AddContinuation(inst);
        }
        else
        {
            FastInterpBoilerplateInstance* inst = thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
                        FastInterpBoilerplateLibrary<FIPartialInlineLhsPointerArithmeticImpl>::SelectBoilerplateBluePrint(
                            m_index->GetTypeId().GetDefaultFastInterpTypeId() /*indexType*/,
                            m_isAddition ? AstArithmeticExprType::ADD : AstArithmeticExprType::SUB,
                            !spillLoc.IsNoSpill(),
                            thread_pochiVMContext->m_fastInterpStackFrameManager->GetNumNoSpillIntegral(),
                            FIOpaqueParamsHelper::GetMaxOFP(),
                            po2Size));
            spillLoc.PopulatePlaceholderIfSpill(inst, 0);
            lhsSpillLoc.PopulatePlaceholderIfSpill(inst, 1);
            if (po2Size == FIPowerOfTwoObjectSize::NOT_POWER_OF_TWO)
            {
                inst->PopulateConstantPlaceholder<uint64_t>(2, objectSize);
            }
            return lhsSnippet.AddContinuation(rhsSnippet).AddContinuation(inst);
        }
    }
}

}   // namespace PochiVM
