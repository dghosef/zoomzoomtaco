#include "codegen/compile_to_pochi.h"
#ifndef DONT_RECOMPILE

// codegen_c
PochiCompiler::PochiSum
PochiCompiler::visit_pochi(const taco::ir::GetProperty *op) {
  taco_iassert(varMap.count(op) > 0) << "Property " << Expr(op) << " of "
                                     << op->tensor << " not found in varMap";
  return pochiVarMap.at(varMap[op]);
}
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Var *op) {
  if (varNames.contains(op)) {
    return pochiVarMap.at(varNames.get(op));
  } else {
    return pochiVarMap.at(op->name);
  }
}
// Codegen_c
PochiCompiler::PochiSum
PochiCompiler::visit_pochi(const taco::ir::Allocate *op) {
  return std::visit(
      [&](auto &&var, auto &&num_elements) -> PochiSum {
        using VarType = std::decay_t<decltype(var)>;
        using VarInnerType = inner_type_t<VarType>;

        using NumElementsType = std::decay_t<decltype(num_elements)>;
        using NumElementsInnerType = inner_type_t<NumElementsType>;
        if constexpr (is_pochi_reference_v<VarType> &&
                      std::is_pointer_v<VarInnerType> &&
                      PochiVM::AstTypeHelper::may_static_cast<
                          NumElementsInnerType, unsigned long>::value &&
                      std::is_integral_v<NumElementsInnerType> &&
                      !std::is_same_v<NumElementsInnerType, bool>) {
          auto size = PochiVM::StaticCast<unsigned long>(
              num_elements *
              static_cast<NumElementsInnerType>(op->var.type().getNumBytes()));
          if (op->is_realloc) {
            return PochiVM::Assign(
                var, PochiVM::ReinterpretCast<VarInnerType>(
                         PochiVM::CallFreeFn::pochi_realloc(
                             PochiVM::StaticCast<void *>(var), size)));
          } else if (op->clear) {
            return PochiVM::Assign(
                var, PochiVM::ReinterpretCast<VarInnerType>(
                         PochiVM::CallFreeFn::pochi_calloc(
                             PochiVM::Literal<unsigned long>(1), size)));
          } else {
            return PochiVM::Assign(
                var, PochiVM::ReinterpretCast<VarInnerType>(
                         PochiVM::CallFreeFn::pochi_malloc(size)));
          }
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->var), visit_taco_op(op->num_elements));
}
// codegen_c.cpp
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Assign *op) {
  return std::visit(
      [&](auto &&lhs, auto &&rhs) -> PochiSum {
        using LhsType = std::decay_t<decltype(lhs)>;
        using LhsInnerType = inner_type_t<LhsType>;

        using RhsType = std::decay_t<decltype(rhs)>;
        using RhsInnerType = inner_type_t<RhsType>;
        if constexpr (is_pochi_reference_v<LhsType> &&
                      has_value_ptr_v<RhsType>) {
          return PochiVM::Value<void>(new PochiVM::AstAssignExpr(
              lhs.__pochivm_ref_ptr, rhs.__pochivm_value_ptr));
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->lhs), visit_taco_op(op->rhs));
}
// codegen_c.cpp
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Store *op) {
  // op->arr, op->loc, op->data
  // Change to std::optional
  // TODO: Add typechecking
  auto ref_ptr = std::visit(
      [&](auto &&arr, auto &&loc) -> PochiVM::AstNodeBase * {
        using ArrType = std::decay_t<decltype(arr)>;
        using InnerArrType = inner_type_t<ArrType>;

        using LocType = std::decay_t<decltype(loc)>;
        using InnerLocType = inner_type_t<LocType>;

        if constexpr (std::is_pointer_v<InnerArrType> &&
                      !std::is_same_v<InnerArrType, void *> &&
                      std::is_integral_v<InnerLocType> &&
                      std::is_arithmetic_v<
                          std::remove_pointer_t<InnerArrType>>) {
          return arr[loc].__pochivm_ref_ptr;
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->arr), visit_taco_op(op->loc));
  return std::visit(
      [&](auto &&data) -> PochiSum {
        using DataType = std::decay_t<decltype(data)>;
        using InnerDataType = inner_type_t<DataType>;
        if constexpr (std::is_arithmetic_v<InnerDataType>) {
          return PochiVM::Value<void>(
              new PochiVM::AstAssignExpr(ref_ptr, data.__pochivm_value_ptr));
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->data));
}
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Load *op) {
  return std::visit(
      [&](auto &&arr, auto &&loc) -> PochiSum {
        using ArrType = std::decay_t<decltype(arr)>;
        using InnerArrType = inner_type_t<ArrType>;

        using LocType = std::decay_t<decltype(loc)>;
        using InnerLocType = inner_type_t<LocType>;

        if constexpr (std::is_pointer_v<InnerArrType> &&
                      !std::is_same_v<InnerArrType, void *> &&
                      std::is_integral_v<InnerLocType>) {
          return arr[loc];
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->arr), visit_taco_op(op->loc));
}
template <typename T>
static PochiCompiler::PochiSum cast_helper(PochiCompiler::PochiSum a) {
  return std::visit(
      [&](auto &&arg) -> PochiCompiler::PochiSum {
        using PochiType = std::decay_t<decltype(arg)>;
        using InnerType = inner_type_t<PochiType>;
        if constexpr (PochiVM::AstTypeHelper::may_static_cast<
                          T, InnerType>::value) {
          return PochiVM::StaticCast<T>(arg);
        } else if constexpr (is_pointer_v<T> && is_pointer_v<InnerType>) {
          return PochiVM::ReinterpretCast<T>(arg);
        } else {
          { taco_uerror << "Invalid Type for Neg"; }
          __builtin_unreachable();
        }
      },
      a);
}

PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Cast *op) {
#define stmt(PochiType, type) return cast_helper<type>(visit_taco_op(op->a));
  switch_over_Datatype(op->type.getKind())
#undef stmt
}
// codegen_c
PochiCompiler::PochiSum
PochiCompiler::visit_pochi(const taco::ir::VarDecl *op) {
  return std::visit(
      [&](auto &&lhs, auto &&rhs) -> PochiSum {
        using LhsType = std::decay_t<decltype(lhs)>;
        using LhsInnerType = inner_type_t<LhsType>;

        using RhsType = std::decay_t<decltype(rhs)>;
        using RhsInnerType = inner_type_t<RhsType>;
        if constexpr (is_pochi_var_v<LhsType> && has_value_ptr_v<RhsType>) {
          return PochiVM::Block(
              PochiVM::Value<void>(new PochiVM::AstAssignExpr(
                  lhs.__pochivm_ref_ptr, rhs.__pochivm_value_ptr)));
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->var), visit_taco_op(op->rhs));
}
#endif