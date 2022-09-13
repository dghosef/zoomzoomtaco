#include "codegen/compile_to_pochi.h"
#ifndef DONT_RECOMPILE
PochiCompiler::PochiSum
PochiCompiler::visit_pochi(const taco::ir::IfThenElse *op) {
  taco_iassert(op->cond.defined());
  taco_iassert(op->then.defined());
  auto ifWithoutElse = std::visit(
      [&](auto &&cond, auto &&then) -> PochiVM::IfStatement {
        using CondType = std::decay_t<decltype(cond)>;
        using InnerCondType = inner_type_t<CondType>;
        using ThenType = std::decay_t<decltype(then)>;
        using InnerThenType = inner_type_t<ThenType>;
        if constexpr (std::is_same_v<InnerCondType, bool> &&
                      std::is_same_v<InnerThenType, void>) {
          return PochiVM::If(cond).Then(then);
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->cond),
      visit_taco_op(Stmt(to<Scope>(op->then)->scopedStmt)));
  if (op->otherwise.defined()) {
    return std::visit(
        [&](auto &&otherwise) -> PochiSum {
          using OtherwiseType = std::decay_t<decltype(otherwise)>;
          using InnerOtherwiseType = inner_type_t<OtherwiseType>;
          if constexpr (std::is_same_v<InnerOtherwiseType, void>) {
            return ifWithoutElse.Else(otherwise);
          } else {
            { taco_uerror << "Invalid Type for Add"; }
            __builtin_unreachable();
          }
        },
        visit_taco_op(op->otherwise));
  } else {
    return ifWithoutElse;
  }
}

PochiVM::IfStatement
PochiCompiler::case_visitor_helper(const taco::ir::Case *op,
                                   size_t curr_clause) {
  auto clause = op->clauses[curr_clause];
  return std::visit(
      [&](auto &&cond, auto &&then) -> PochiVM::IfStatement {
        using CondType = std::decay_t<decltype(cond)>;
        using InnerCondType = inner_type_t<CondType>;
        using ThenType = std::decay_t<decltype(then)>;
        using InnerThenType = inner_type_t<ThenType>;
        if constexpr (std::is_same_v<InnerCondType, bool> &&
                      std::is_same_v<InnerThenType, void>) {
          if (curr_clause == op->clauses.size() - 1) {
            return PochiVM::If(cond).Then(then);
          } else {
            return PochiVM::If(cond).Then(then).Else(
                case_visitor_helper(op, curr_clause + 1));
          }
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(clause.first), visit_taco_op(clause.second));
}

PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Case *op) {
  return case_visitor_helper(op, 0);
}
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Switch *op) {
  return std::visit(
      [&](auto &&controlExprValue) -> PochiSum {
        using ExprType = std::decay_t<decltype(controlExprValue)>;
        using InnerExprType = inner_type_t<ExprType>;
        if constexpr (std::is_arithmetic_v<InnerExprType>) {
          return switch_visitor_helper(op, 0, controlExprValue);
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->controlExpr));
}
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::For *op) {
  return std::visit(
      [&](auto &&var) -> PochiSum {
        using VarType = std::decay_t<decltype(var)>;
        using InnerVarType = inner_type_t<VarType>;
        if constexpr (std::is_integral_v<InnerVarType> &&
                      is_pochi_var_v<VarType>) {
          PochiVM::Scope ret = PochiVM::Scope();
          auto start = visit_taco_op(op->start);
          auto end = visit_taco_op(op->end);
          PochiVM::AstNodeBase *var_value_ptr = var.__pochivm_value_ptr;
          PochiVM::AstNodeBase *var_reference_ptr = var.__pochivm_ref_ptr;
          PochiVM::AstNodeBase *start_value_ptr = get_value_pointer(start);
          PochiVM::AstNodeBase *end_value_ptr = get_value_pointer(end);
          PochiVM::Value<void> assign_stmt = PochiVM::Value<void>(
              new PochiVM::AstAssignExpr(var_reference_ptr, start_value_ptr));
          PochiVM::Value<bool> comparison_expr =
              PochiVM::Value<bool>(new PochiVM::AstComparisonExpr(
                  PochiVM::AstComparisonExprType::LESS_THAN, var_value_ptr,
                  end_value_ptr));
          auto lit = op->increment.as<Literal>();
          PochiVM::AstNodeBase *value_to_add = nullptr;
          if (lit != nullptr &&
              ((lit->type.isInt() && lit->equalsScalar(1)) ||
               (lit->type.isUInt() && lit->equalsScalar(1)))) {
            value_to_add =
                PochiVM::Literal<InnerVarType>(1).__pochivm_value_ptr;
          } else {
            value_to_add = std::visit(
                [&](auto &&inc) -> PochiVM::AstNodeBase * {
                  if constexpr (!std::is_same_v<
                                    std::decay_t<decltype(inc)>,
                                    PochiVM::Reference<taco_tensor_t>>) {
                    return inc.__pochivm_value_ptr;
                  } else {
                    { taco_uerror << "Invalid Type for Add"; }
                    __builtin_unreachable();
                  }
                },
                visit_taco_op(op->increment));
          }
          PochiVM::Value<void> increment_stmt =
              PochiVM::Value<void>(new PochiVM::AstAssignExpr(
                  var_reference_ptr,
                  PochiVM::Value<InnerVarType>(
                      new PochiVM::AstArithmeticExpr(
                          PochiVM::AstArithmeticExprType::ADD, var_value_ptr,
                          value_to_add))
                      .__pochivm_value_ptr));
          return std::visit(
              [&](auto &&contents) -> PochiSum {
                if constexpr (std::is_same_v<inner_type_t<std::decay_t<
                                                 decltype(contents)>>,
                                             void>) {
                  ret.Append(PochiVM::For(assign_stmt, comparison_expr,
                                      increment_stmt)
                      .Do(contents));
                  return ret;
                } else {
                  {
                    taco_uerror << "Invalid Type. Got "
                                << typeid(contents).name() << " expected void";
                  }
                  __builtin_unreachable();
                }
              },
              visit_taco_op(op->contents));
        }

        else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->var));
}
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::While *op) {
  return std::visit(
      [&](auto &&cond, auto &&contents) -> PochiSum {
        using CondType = std::decay_t<decltype(cond)>;
        using InnerCondType = inner_type_t<CondType>;
        using ContentsType = std::decay_t<decltype(contents)>;
        using InnerContentsType = inner_type_t<ContentsType>;
        if constexpr (std::is_same_v<InnerCondType, bool> &&
                      std::is_same_v<InnerContentsType, void>) {
          return PochiVM::While(cond).Do(contents);
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->cond), visit_taco_op(op->contents));
}
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Block *op) {
  PochiVM::Block block;
  for (taco::ir::Stmt stmt : op->contents) {
    block.Append(visit_taco_stmt(visit_taco_op(stmt)));
  }
  return block;
}
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Scope *op) {
  varNames.scope();
  auto ret = PochiVM::Scope(visit_taco_stmt(visit_taco_op(op->scopedStmt)));
  varNames.unscope();
  return ret;
}

// codegen_c.cpp
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Yield *op) {
  { taco_uerror << "Coroutines not supported with PochiVM backend yet"; }
  __builtin_unreachable();
}
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Break *op) {
  return PochiVM::Break();
}
PochiCompiler::PochiSum
PochiCompiler::visit_pochi(const taco::ir::Comment *op) {
  silence_noret;
}
PochiCompiler::PochiSum
PochiCompiler::visit_pochi(const taco::ir::BlankLine *op) {
  silence_noret;
}
PochiCompiler::PochiSum
PochiCompiler::visit_pochi(const taco::ir::Continue *op) {
  return PochiVM::Continue();
}
#endif