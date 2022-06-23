#include "codegen/compile_to_pochi.h"
#ifndef DONT_RECOMPILE
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Neg *op) {
  return std::visit(
      [&](auto &&arg) -> PochiSum {
        using PochiType = std::decay_t<decltype(arg)>;
        using InnerType = inner_type_t<PochiType>;
        if constexpr (std::is_same_v<InnerType, bool>) {
          return !arg;
        } else if constexpr (std::is_arithmetic_v<InnerType>) {
          return arg * PochiVM::Literal<InnerType>(-1);
        } else {
          { taco_uerror << "Invalid Type for Neg"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->a));
}

PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Add *op) {
  return std::visit(
      [&](auto &&arg1, auto &&arg2) -> PochiSum {
        using PochiType1 = std::decay_t<decltype(arg1)>;
        using InnerType1 = inner_type_t<PochiType1>;
        using PochiType2 = std::decay_t<decltype(arg2)>;
        using InnerType2 = inner_type_t<PochiType2>;
        if constexpr (!std::is_same_v<InnerType1, bool> &&
                      std::is_same_v<InnerType1, InnerType2> &&
                      std::is_arithmetic_v<InnerType1>) {
          return arg1 + arg2;
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->a), visit_taco_op(op->b));
}
// TODO: Switch to correct op
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Sub *op) {
  return std::visit(
      [&](auto &&arg1, auto &&arg2) -> PochiSum {
        using PochiType1 = std::decay_t<decltype(arg1)>;
        using InnerType1 = inner_type_t<PochiType1>;
        using PochiType2 = std::decay_t<decltype(arg2)>;
        using InnerType2 = inner_type_t<PochiType2>;
        if constexpr (!std::is_same_v<InnerType1, bool> &&
                      std::is_same_v<InnerType1, InnerType2> &&
                      std::is_arithmetic_v<InnerType1>) {
          return arg1 - arg2;
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->a), visit_taco_op(op->b));
}
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Mul *op) {
  return std::visit(
      [&](auto &&arg1, auto &&arg2) -> PochiSum {
        using PochiType1 = std::decay_t<decltype(arg1)>;
        using InnerType1 = inner_type_t<PochiType1>;
        using PochiType2 = std::decay_t<decltype(arg2)>;
        using InnerType2 = inner_type_t<PochiType2>;
        if constexpr (!std::is_same_v<InnerType1, bool> &&
                      std::is_same_v<InnerType1, InnerType2> &&
                      std::is_arithmetic_v<InnerType1>) {
          return arg1 * arg2;
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->a), visit_taco_op(op->b));
}
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Div *op) {
  return std::visit(
      [&](auto &&arg1, auto &&arg2) -> PochiSum {
        using PochiType1 = std::decay_t<decltype(arg1)>;
        using InnerType1 = inner_type_t<PochiType1>;
        using PochiType2 = std::decay_t<decltype(arg2)>;
        using InnerType2 = inner_type_t<PochiType2>;
        if constexpr (!std::is_same_v<InnerType1, bool> &&
                      std::is_same_v<InnerType1, InnerType2> &&
                      std::is_arithmetic_v<InnerType1>) {
          return arg1 / arg2;
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->a), visit_taco_op(op->b));
}
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Rem *op) {
  return std::visit(
      [&](auto &&arg1, auto &&arg2) -> PochiSum {
        using PochiType1 = std::decay_t<decltype(arg1)>;
        using InnerType1 = inner_type_t<PochiType1>;
        using PochiType2 = std::decay_t<decltype(arg2)>;
        using InnerType2 = inner_type_t<PochiType2>;
        if constexpr (!std::is_same_v<InnerType1, bool> &&
                      std::is_same_v<InnerType1, InnerType2> &&
                      std::is_integral_v<InnerType1>) {
          return arg1 % arg2;
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->a), visit_taco_op(op->b));
}
#endif