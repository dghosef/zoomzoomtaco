#include "codegen/compile_to_pochi.h"
#ifndef DONT_RECOMPILE

PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::And *op) {
  return std::visit(
      [&](auto &&arg1, auto &&arg2) -> PochiSum {
        using PochiType1 = std::decay_t<decltype(arg1)>;
        using InnerType1 = inner_type_t<PochiType1>;
        using PochiType2 = std::decay_t<decltype(arg2)>;
        using InnerType2 = inner_type_t<PochiType2>;
        if constexpr (std::is_same_v<InnerType1, InnerType2> &&
                      std::is_same_v<InnerType1, bool>) {
          return PochiVM::Value<bool>(new PochiVM::AstLogicalAndOrExpr(
              true, arg1.__pochivm_value_ptr, arg2.__pochivm_value_ptr));
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->a), visit_taco_op(op->b));
}
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Or *op) {
  return std::visit(
      [&](auto &&arg1, auto &&arg2) -> PochiSum {
        using PochiType1 = std::decay_t<decltype(arg1)>;
        using InnerType1 = inner_type_t<PochiType1>;
        using PochiType2 = std::decay_t<decltype(arg2)>;
        using InnerType2 = inner_type_t<PochiType2>;
        if constexpr (std::is_same_v<InnerType1, InnerType2> &&
                      std::is_same_v<InnerType1, bool>) {
          return PochiVM::Value<bool>(new PochiVM::AstLogicalAndOrExpr(
              false, arg1.__pochivm_value_ptr, arg2.__pochivm_value_ptr));
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->a), visit_taco_op(op->b));
}
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::BitAnd *op) {
  return std::visit(
      [&](auto &&arg1, auto &&arg2) -> PochiSum {
        using PochiType1 = std::decay_t<decltype(arg1)>;
        using InnerType1 = inner_type_t<PochiType1>;
        using PochiType2 = std::decay_t<decltype(arg2)>;
        using InnerType2 = inner_type_t<PochiType2>;
        if constexpr (!std::is_same_v<InnerType1, bool> &&
                      std::is_same_v<InnerType1, InnerType2> &&
                      PochiVM::AstTypeHelper::may_static_cast<InnerType2,
                                                              long>::value) {
          return PochiVM::StaticCast<InnerType1>(
              PochiVM::CallFreeFn::pochi_bitand(
                  PochiVM::StaticCast<long>(arg1),
                  PochiVM::StaticCast<long>(arg2)));
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->a), visit_taco_op(op->b));
}
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::BitOr *op) {
  return std::visit(
      [&](auto &&arg1, auto &&arg2) -> PochiSum {
        using PochiType1 = std::decay_t<decltype(arg1)>;
        using InnerType1 = inner_type_t<PochiType1>;
        using PochiType2 = std::decay_t<decltype(arg2)>;
        using InnerType2 = inner_type_t<PochiType2>;
        if constexpr (!std::is_same_v<InnerType1, bool> &&
                      std::is_same_v<InnerType1, InnerType2> &&
                      PochiVM::AstTypeHelper::may_static_cast<InnerType2,
                                                              long>::value) {
          return PochiVM::StaticCast<InnerType1>(
              PochiVM::CallFreeFn::pochi_bitor(
                  PochiVM::StaticCast<long>(arg1),
                  PochiVM::StaticCast<long>(arg2)));
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->a), visit_taco_op(op->b));
}
#endif
