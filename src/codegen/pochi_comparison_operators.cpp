#include "codegen/compile_to_pochi.h"
#ifndef DONT_RECOMPILE

PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Eq *op) {
  return std::visit(
      [&](auto &&arg1, auto &&arg2) -> PochiSum {
        using PochiType1 = std::decay_t<decltype(arg1)>;
        using InnerType1 = inner_type_t<PochiType1>;
        using PochiType2 = std::decay_t<decltype(arg2)>;
        using InnerType2 = inner_type_t<PochiType2>;
        if constexpr (std::is_same_v<InnerType1, InnerType2> &&
                      !std::is_pointer_v<InnerType1> &&
                      std::is_arithmetic_v<InnerType1>) {
          return PochiVM::Value<bool>(new PochiVM::AstComparisonExpr(
              PochiVM::AstComparisonExprType::EQUAL, arg1.__pochivm_value_ptr,
              arg2.__pochivm_value_ptr));
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->a), visit_taco_op(op->b));
}
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Neq *op) {
  return std::visit(
      [&](auto &&arg1, auto &&arg2) -> PochiSum {
        using PochiType1 = std::decay_t<decltype(arg1)>;
        using InnerType1 = inner_type_t<PochiType1>;
        using PochiType2 = std::decay_t<decltype(arg2)>;
        using InnerType2 = inner_type_t<PochiType2>;
        if constexpr (std::is_same_v<InnerType1, InnerType2> &&
                      !std::is_pointer_v<InnerType1> &&
                      std::is_arithmetic_v<InnerType1>) {
          return PochiVM::Value<bool>(new PochiVM::AstComparisonExpr(
              PochiVM::AstComparisonExprType::NOT_EQUAL,
              arg1.__pochivm_value_ptr, arg2.__pochivm_value_ptr));
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->a), visit_taco_op(op->b));
}
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Gt *op) {
  return std::visit(
      [&](auto &&arg1, auto &&arg2) -> PochiSum {
        using PochiType1 = std::decay_t<decltype(arg1)>;
        using InnerType1 = inner_type_t<PochiType1>;
        using PochiType2 = std::decay_t<decltype(arg2)>;
        using InnerType2 = inner_type_t<PochiType2>;
        if constexpr (std::is_same_v<InnerType1, InnerType2> &&
                      !std::is_pointer_v<InnerType1> &&
                      std::is_arithmetic_v<InnerType1> &&
                      !std::is_same_v<InnerType1, bool>) {
          return PochiVM::Value<bool>(new PochiVM::AstComparisonExpr(
              PochiVM::AstComparisonExprType::GREATER_THAN,
              arg1.__pochivm_value_ptr, arg2.__pochivm_value_ptr));
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->a), visit_taco_op(op->b));
}
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Lt *op) {
  return std::visit(
      [&](auto &&arg1, auto &&arg2) -> PochiSum {
        using PochiType1 = std::decay_t<decltype(arg1)>;
        using InnerType1 = inner_type_t<PochiType1>;
        using PochiType2 = std::decay_t<decltype(arg2)>;
        using InnerType2 = inner_type_t<PochiType2>;
        if constexpr (std::is_same_v<InnerType1, InnerType2> &&
                      !std::is_pointer_v<InnerType1> &&
                      !std::is_same_v<InnerType1, void> &&
                      std::is_arithmetic_v<InnerType1> &&
                      !std::is_same_v<InnerType1, bool>) {
          return PochiVM::Value<bool>(new PochiVM::AstComparisonExpr(
              PochiVM::AstComparisonExprType::LESS_THAN,
              arg1.__pochivm_value_ptr, arg2.__pochivm_value_ptr));
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->a), visit_taco_op(op->b));
}
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Gte *op) {
  return std::visit(
      [&](auto &&arg1, auto &&arg2) -> PochiSum {
        using PochiType1 = std::decay_t<decltype(arg1)>;
        using InnerType1 = inner_type_t<PochiType1>;
        using PochiType2 = std::decay_t<decltype(arg2)>;
        using InnerType2 = inner_type_t<PochiType2>;
        if constexpr (std::is_same_v<InnerType1, InnerType2> &&
                      !std::is_pointer_v<InnerType1> &&
                      !std::is_same_v<InnerType1, void> &&
                      std::is_arithmetic_v<InnerType1> &&
                      !std::is_same_v<InnerType1, bool>) {
          return PochiVM::Value<bool>(new PochiVM::AstComparisonExpr(
              PochiVM::AstComparisonExprType::GREATER_EQUAL,
              arg1.__pochivm_value_ptr, arg2.__pochivm_value_ptr));
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->a), visit_taco_op(op->b));
}
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Lte *op) {
  return std::visit(
      [&](auto &&arg1, auto &&arg2) -> PochiSum {
        using PochiType1 = std::decay_t<decltype(arg1)>;
        using InnerType1 = inner_type_t<PochiType1>;
        using PochiType2 = std::decay_t<decltype(arg2)>;
        using InnerType2 = inner_type_t<PochiType2>;
        if constexpr (std::is_same_v<InnerType1, InnerType2> &&
                      !std::is_pointer_v<InnerType1> &&
                      !std::is_same_v<InnerType1, void> &&
                      std::is_arithmetic_v<InnerType1> &&
                      !std::is_same_v<InnerType1, bool>) {
          return PochiVM::Value<bool>(new PochiVM::AstComparisonExpr(
              PochiVM::AstComparisonExprType::LESS_EQUAL,
              arg1.__pochivm_value_ptr, arg2.__pochivm_value_ptr));
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->a), visit_taco_op(op->b));
}
#endif