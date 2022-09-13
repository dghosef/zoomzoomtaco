#pragma once

#include "ast_expr_base.h"
#include "common_expr.h"
#include "pochivm_context.h"
#include "ast_variable.h"
#include "destructor_helper.h"

namespace PochiVM
{

// increment if equal
// TODO: Just make this a supernode of assign
//
class AstIncrIfEqStatement : public AstNodeBase
{
    public:
    AstIncrIfEqStatement(AstNodeBase* dst, AstNodeBase* lhs, AstNodeBase* rhs)
        : AstNodeBase(AstNodeType::AstIncrIfEqStatement, TypeId::Get<void>())
        , m_dst(dst), m_lhs(lhs), m_rhs(rhs)
    {
    // TODO: Add asserts to typecheck lhs/rhs/dst
    }

    template<typename SrcT>
    void InterpImpl(void* /*out*/)
    {
        assert(false); // haven't done this yet
    }

    template<typename ...Targs>
    void SelectImpl(void*) {
        assert(false); // Only supports fastinterp for now
    }

    AstNodeBase* GetDst() const { return m_dst; }

    virtual void SetupDebugInterpImpl() override final
    {
        assert(false); // Only supports fastinterp for now
    }

    virtual void ForEachChildren(FunctionRef<void(AstNodeBase*&)> fn) override final
    {
        fn(m_dst);
        fn(m_lhs);
        fn(m_rhs);
    }

    virtual llvm::Value* WARN_UNUSED EmitIRImpl() override final {
        assert(false);
        return nullptr;
    }

    virtual FastInterpSnippet WARN_UNUSED PrepareForFastInterp(FISpillLocation spillLoc) override final;
    virtual void FastInterpSetupSpillLocation() override final;

private:
    AstNodeBase* m_dst;
    AstNodeBase* m_lhs;
    AstNodeBase* m_rhs;
};
// increment if equal
//
class AstIncrStatement : public AstNodeBase
{
    public:
    AstIncrStatement(AstNodeBase* dst)
        : AstNodeBase(AstNodeType::AstIncrStatement, TypeId::Get<void>())
        , m_dst(dst)
    {
        assert(dst->GetAstNodeType() == AstNodeType::AstVariable);
        assert(dst->GetTypeId().IsPointerType() || dst->GetTypeId().IsFloatingPoint() || dst->GetTypeId().IsPrimitiveIntType());
    }

    template<typename SrcT>
    void InterpImpl(void* /*out*/)
    {
        assert(false); // haven't done this yet
    }

    template<typename ...Targs>
    void SelectImpl(void*) {
        assert(false); // Only supports fastinterp for now
    }

    AstNodeBase* GetDst() const { return m_dst; }

    virtual void SetupDebugInterpImpl() override final
    {
        assert(false); // Only supports fastinterp for now
    }

    virtual void ForEachChildren(FunctionRef<void(AstNodeBase*&)> fn) override final
    {
        fn(m_dst);
    }

    virtual llvm::Value* WARN_UNUSED EmitIRImpl() override final {
        assert(false);
        return nullptr;
    }

    virtual FastInterpSnippet WARN_UNUSED PrepareForFastInterp(FISpillLocation spillLoc) override final;
    virtual void FastInterpSetupSpillLocation() override final;

private:
    AstNodeBase* m_dst;
};
class AstGotoStmt : public AstNodeBase
{
public:
    AstGotoStmt(std::string label)
        : AstNodeBase(AstNodeType::AstGotoStmt, TypeId::Get<void>())
        , m_isBreak(true), m_label(label)
    {}


    void InterpImpl(InterpControlSignal* ics)
    {
        assert(ics != nullptr && *ics == InterpControlSignal::None);
        if (m_isBreak)
        {
            *ics = InterpControlSignal::Break;
        }
        else
        {
            *ics = InterpControlSignal::Continue;
        }
    }

    bool IsBreakStatement() const { return m_isBreak; }

    virtual void SetupDebugInterpImpl() override final
    {
        m_debugInterpFn = AstTypeHelper::GetClassMethodPtr(&AstGotoStmt::InterpImpl);
    }

    virtual void ForEachChildren(FunctionRef<void(AstNodeBase*&)> /*fn*/) override final { }

    virtual FastInterpSnippet WARN_UNUSED PrepareForFastInterp(FISpillLocation spillLoc) override final;
    virtual void FastInterpSetupSpillLocation() override final { }

    virtual llvm::Value* WARN_UNUSED EmitIRImpl() override final {
        assert(false);
        return nullptr;
    }
private:
    // whether it is a break statement or a continue statement
    //
    bool m_isBreak;
    std::string m_label;
};
class AstLabelStmt : public AstNodeBase
{
public:
    AstLabelStmt(std::string label)
        : AstNodeBase(AstNodeType::AstLabelStmt, TypeId::Get<void>()), m_label(label)
    { }


    void InterpImpl(InterpControlSignal* /*ics*/)
    {
        assert(false);
    }

    //hi :)


    virtual void SetupDebugInterpImpl() override final
    {
        m_debugInterpFn = AstTypeHelper::GetClassMethodPtr(&AstLabelStmt::InterpImpl);
    }

    virtual void ForEachChildren(FunctionRef<void(AstNodeBase*&)> /*fn*/) override final { }

    virtual FastInterpSnippet WARN_UNUSED PrepareForFastInterp(FISpillLocation spillLoc) override final ;
    // Just a noop - no spill
    virtual void FastInterpSetupSpillLocation() override final { }

    virtual llvm::Value* WARN_UNUSED EmitIRImpl() override final {
        assert(false);
        return nullptr;
    }
private:
std::string m_label;
};
}   // namespace PochiVM
