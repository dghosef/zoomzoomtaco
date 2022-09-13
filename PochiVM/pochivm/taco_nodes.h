#pragma once

#include "api_base.h"
#include "lang_constructs.h"
#include "taco_ast.h"

namespace PochiVM {
template <typename T>
inline Value<void> IncrIfEq(const Reference<T> &dst, const Value<T> &lhs,
                          const Value<T> &rhs) {
  return Value<void>(new AstIncrIfEqStatement(
      dst.__pochivm_ref_ptr, lhs.__pochivm_value_ptr, rhs.__pochivm_value_ptr));
}
inline Value<void> IncrIfEq(const ReferenceVT &dst, const ValueVT &lhs,
                          const ValueVT &rhs) {
  return Value<void>(new AstIncrIfEqStatement(
      dst.__pochivm_ref_ptr, lhs.__pochivm_value_ptr, rhs.__pochivm_value_ptr));
}
inline Value<void> Goto(std::string label) {
  return Value<void>(new AstGotoStmt(label));
}
inline Value<void> Label(std::string label) {
    return Value<void>(new AstLabelStmt(label));
}
template <typename T>
inline Value<void> IncrementVar(const Reference<T> &var) {
    return Value<void>(new AstIncrStatement(var.__pochivm_ref_ptr));
}
inline Value<void> IncrementVar(const ReferenceVT &var) {
    return Value<void>(new AstIncrStatement(var.__pochivm_ref_ptr));
}
} // namespace PochiVM
