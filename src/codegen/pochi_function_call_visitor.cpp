#include "codegen/compile_to_pochi.h"
#ifndef DONT_RECOMPILE

PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Malloc *op) {
  return std::visit(
      [&](auto &&size) -> PochiSum {
        using SizeType = std::decay_t<decltype(size)>;
        using InnerSizeType = inner_type_t<SizeType>;
        if constexpr (PochiVM::AstTypeHelper::may_static_cast<
                          InnerSizeType, unsigned long>::value) {
          auto casted_size =
              PochiVM::Value<unsigned long>(new PochiVM::AstStaticCastExpr(
                  size.__pochivm_value_ptr,
                  PochiVM::TypeId::Get<unsigned long>()));
          return PochiVM::CallFreeFn::pochi_malloc(casted_size);
        }
        { taco_uerror << "Invalid Type for Add"; }
        __builtin_unreachable();
      },
      visit_taco_op(op->size));
}
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Sizeof *op) {
  size_t arr_size = 1;
  for (Dimension dimension : op->sizeofType.getShape()) {
    arr_size *= dimension.getSize();
  }
  if (arr_size == 0) {
    taco_uwarning << "trying to get sizeof of variable size dimension thingy";
    arr_size = 1;
  }
  return PochiVM::Literal<size_t>(op->sizeofType.getDataType().getNumBytes() *
                                  arr_size);
}

PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Free *op) {
  return std::visit(
      [&](auto &&var) -> PochiSum {
        using VarType = std::decay_t<decltype(var)>;
        using VarInnerType = inner_type_t<VarType>;
        if constexpr (is_pointer_v<VarInnerType>) {
          return PochiVM::CallFreeFn::pochi_free(
              PochiVM::ReinterpretCast<void *>(var));
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->var));
}

PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Sort *op) {
  return std::visit(
      [&](auto &&base, auto &&num_elems) -> PochiSum {
        using BaseType = std::decay_t<decltype(base)>;
        using BaseInnerType = inner_type_t<BaseType>;
        using NumElemsType = std::decay_t<decltype(num_elems)>;
        using NumElemsInnerType = inner_type_t<NumElemsType>;
        if constexpr (std::is_pointer_v<BaseInnerType> &&
                      PochiVM::AstTypeHelper::may_static_cast<
                          NumElemsInnerType, unsigned long>::value) {
          return PochiVM::CallFreeFn::pochi_qsort(
              PochiVM::ReinterpretCast<void *>(base),
              PochiVM::StaticCast<unsigned long>(num_elems));
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->args[0]), visit_taco_op(op->args[1]));
}
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Call *op) {
  unimplemented;
}
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Print *op) {
  { taco_uerror << "Print not supported yet with PochiVM backend"; }
  __builtin_unreachable();
}
// codegen_c
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Sqrt *op) {
  taco_tassert(op->type.isFloat() && op->type.getNumBits() == 64)
      << "Codegen doesn't currently support non-double sqrt";
  return std::visit(
      [&](auto &&arg) -> PochiSum {
        using PochiType = std::decay_t<decltype(arg)>;
        using InnerType = inner_type_t<PochiType>;
        if constexpr (std::is_same_v<double, bool>) {
          return PochiVM::CallFreeFn::sqrt_double(arg);
        } else {
          { taco_uerror << "Invalid Type for Neg"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->a));
}
// codegen_c
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Min *op) {
  return std::visit(
      [&](auto &&arg1, auto &&arg2) -> PochiSum {
        using PochiType1 = std::decay_t<decltype(arg1)>;
        using InnerType1 = inner_type_t<PochiType1>;
        using PochiType2 = std::decay_t<decltype(arg2)>;
        using InnerType2 = inner_type_t<PochiType2>;
        if constexpr (!std::is_same_v<InnerType1, bool> &&
                      std::is_same_v<InnerType1, InnerType2> &&
                      PochiVM::AstTypeHelper::may_static_cast<InnerType2,
                                                              int>::value) {
          return PochiVM::CallFreeFn::min(PochiVM::StaticCast<int>(arg1), PochiVM::StaticCast<int>(arg2));
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->operands[0]), visit_taco_op(op->operands[1]));
}

// codegen_c
PochiCompiler::PochiSum PochiCompiler::visit_pochi(const taco::ir::Max *op) {
  return std::visit(
      [&](auto &&arg1, auto &&arg2) -> PochiSum {
        using PochiType1 = std::decay_t<decltype(arg1)>;
        using InnerType1 = inner_type_t<PochiType1>;
        using PochiType2 = std::decay_t<decltype(arg2)>;
        using InnerType2 = inner_type_t<PochiType2>;
        if constexpr (!std::is_same_v<InnerType1, bool> &&
                      std::is_same_v<InnerType1, InnerType2> &&
                      PochiVM::AstTypeHelper::may_static_cast<InnerType2,
                                                              int>::value) {
          return PochiVM::CallFreeFn::max(PochiVM::StaticCast<int>(arg1), PochiVM::StaticCast<int>(arg2));
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      visit_taco_op(op->operands[0]), visit_taco_op(op->operands[1]));
}
#endif