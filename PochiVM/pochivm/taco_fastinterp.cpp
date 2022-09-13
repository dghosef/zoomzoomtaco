#include "arith_expr.h"
#include "taco_ast.h"
#include "codegen_context.hpp"
#include "destructor_helper.h"
#include "fastinterp_ast_helper.hpp"
#include "logical_operator.h"

namespace PochiVM {
// GOTO is done by:
// snippet.m_tail->PopulateBoilerplateFnPtrPlaceholder(0, target.first);
// target.first is a boilerplate instance *
// Make a map from label to m_tail pointers
// make a map from label to boilerplate instances


// Shouldn't need to do anythnig because this is a fully inlined statement
void AstIncrStatement::FastInterpSetupSpillLocation() { }
FastInterpSnippet WARN_UNUSED AstIncrStatement::PrepareForFastInterp( FISpillLocation TESTBUILD_ONLY(spillLoc)) {
  TestAssert(spillLoc.IsNoSpill());

  // Case var = osc op osc
  // FIFullyInlineAssignArithExprImpl
  //
  // lhs is a variable and rhs is a variable
  // lhs is integral and rhs is integral
  TestAssert(m_dst->GetAstNodeType() == AstNodeType::AstVariable);
  TestAssert(m_dst->GetTypeId().RemovePointer().IsPrimitiveIntType() || m_dst->GetTypeId().RemovePointer().IsPointerType());
  AstVariable *var = assert_cast<AstVariable *>(m_dst);
  FastInterpBoilerplateInstance *inst =
      thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
          FastInterpBoilerplateLibrary<FIIncrementImpl>::
              SelectBoilerplateBluePrint(
                  FastInterpTypeId(m_dst->GetTypeId().RemovePointer()),
                  FIOpaqueParamsHelper::GetMaxOIP(),
                  FIOpaqueParamsHelper::GetMaxOFP()
                  ));
  inst->PopulateConstantPlaceholder<uint64_t>(0, var->GetFastInterpOffset());
  return FastInterpSnippet{inst, inst};

}


// When we create each label, look for gotos whose destination hasn't been populated and populate it
static std::map<std::string, std::vector<FastInterpBoilerplateInstance*>> unpopulated_gotos;

// When we create a goto, check if our label is in label_starts and if so, populate the goto's destination
static std::map<std::string, FastInterpBoilerplateInstance*> label_boilerplates;

// Do Nothing
// These are inlined so I don't think I need to do anything
void AstIncrIfEqStatement::FastInterpSetupSpillLocation() {}

FastInterpSnippet WARN_UNUSED AstIncrIfEqStatement::PrepareForFastInterp(
    FISpillLocation TESTBUILD_ONLY(spillLoc)) {
  TestAssert(spillLoc.IsNoSpill());

  // Case var = osc op osc
  // FIFullyInlineAssignArithExprImpl
  //
  // lhs is a variable and rhs is a variable
  // lhs is integral and rhs is integral
  TestAssert(m_dst->GetAstNodeType() == AstNodeType::AstVariable);
  TestAssert(m_lhs->GetTypeId().IsPrimitiveIntType() &&
             m_rhs->GetTypeId().IsPrimitiveIntType());
  AstFIOperandShape lhs = AstFIOperandShape::TryMatch(m_lhs);
  TestAssert(lhs.MatchOK());
  AstFIOperandShape rhs = AstFIOperandShape::TryMatch(m_rhs);
  TestAssert(rhs.MatchOK());
  AstVariable *var = assert_cast<AstVariable *>(m_dst);
  FastInterpBoilerplateInstance *inst =
      thread_pochiVMContext->m_fastInterpEngine->InstantiateBoilerplate(
          FastInterpBoilerplateLibrary<FIIncrIfEqImpl>::
              SelectBoilerplateBluePrint(
                  m_lhs->GetTypeId().GetOneLevelPtrFastInterpTypeId(),
                  lhs.m_indexType, rhs.m_indexType, lhs.m_kind, rhs.m_kind,
                  FIOpaqueParamsHelper::GetMaxOIP(),
                  FIOpaqueParamsHelper::GetMaxOFP(),
                  AstArithmeticExprType::ADD /*doesn't mean anything*/));
  inst->PopulateConstantPlaceholder<uint64_t>(0, var->GetFastInterpOffset());
  lhs.PopulatePlaceholder(inst, 1, 2);
  rhs.PopulatePlaceholder(inst, 3, 4);
  return FastInterpSnippet{inst, inst};
}
FastInterpSnippet WARN_UNUSED AstGotoStmt::PrepareForFastInterp(FISpillLocation TESTBUILD_ONLY(spillLoc))
{
    TestAssert(spillLoc.IsNoSpill());

    AstNodeBase *scope = thread_pochiVMContext->m_scopedVariableManager.GetCurrentScope();

    FastInterpSnippet snippet = thread_pochiVMContext->m_scopedVariableManager.FIGenerateDestructorSequenceUntilScope(scope);
    TestAssert(!snippet.IsUncontinuable());
    if (snippet.IsEmpty())
    {
        snippet = snippet.AddContinuation(FIGetNoopBoilerplate());
    }
    // BEGIN NEW STUFF  
    if(label_boilerplates.find(m_label) != label_boilerplates.end()) {
        snippet.m_tail->PopulateBoilerplateFnPtrPlaceholder(0, label_boilerplates.at(m_label));
    }
    else {
        unpopulated_gotos[m_label].push_back(snippet.m_tail);
    }
    // END NEW STUFF
    return FastInterpSnippet {
        snippet.m_entry, nullptr
    };
}
// TODO:proper error handling for goto
FastInterpSnippet WARN_UNUSED AstLabelStmt::PrepareForFastInterp(FISpillLocation TESTBUILD_ONLY(spillLoc))
{
    TestAssert(spillLoc.IsNoSpill());
    FastInterpBoilerplateInstance *noop = FIGetNoopBoilerplate();
    noop->SetAlignmentLog2(4);
    TestAssert(noop);
    // BEGIN NEW STUFF
    label_boilerplates[m_label] = noop;
    for(FastInterpBoilerplateInstance* target : unpopulated_gotos[m_label]) {
        target->PopulateBoilerplateFnPtrPlaceholder(0, noop);
    }
    // END NEW STUFF
    return FastInterpSnippet{noop, noop};
  }
}   // namespace PochiVM

