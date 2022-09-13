// OPTIMIZATIONS TO DO:
// TODO:
// figure out coroutine
// Shim function
// Compiling/Calling the generated function

#include "codegen/compile_to_pochi.h"
PochiCompiler::PochiSum
PochiCompiler::visit_pochi(const taco::ir::Literal *op) {
  switch (op->type.getKind()) {
  case taco::Datatype::Bool:
    return PochiVM::Literal<bool>(op->getValue<bool>());
  case taco::Datatype::UInt8:
    return PochiVM::Literal<uint8_t>(
        static_cast<uint16_t>(op->getValue<uint8_t>()));
  case taco::Datatype::UInt16:
    return PochiVM::Literal<uint16_t>(op->getValue<uint16_t>());
  case taco::Datatype::UInt32:
    return PochiVM::Literal<uint32_t>(op->getValue<uint32_t>());
  case taco::Datatype::UInt64:
    return PochiVM::Literal<uint64_t>(op->getValue<uint64_t>());
  case taco::Datatype::UInt128:
    taco_not_supported_yet;
    // To silence warning
    return PochiVM::Literal<bool>(false);
    break;
  case taco::Datatype::Int8:
    return PochiVM::Literal<int8_t>(
        static_cast<int16_t>(op->getValue<int8_t>()));
  case taco::Datatype::Int16:
    return PochiVM::Literal<int16_t>(op->getValue<int16_t>());
  case taco::Datatype::Int32:
    return PochiVM::Literal<int32_t>(op->getValue<int32_t>());
  case taco::Datatype::Int64:
    return PochiVM::Literal<int64_t>(op->getValue<int64_t>());
  case taco::Datatype::Int128:
    taco_not_supported_yet;
    return PochiVM::Literal<bool>(false);
    break;
  case taco::Datatype::Float32:
    return PochiVM::Literal<float>(op->getValue<float>());
  case taco::Datatype::Float64:
    return PochiVM::Literal<double>(op->getValue<double>());
  case taco::Datatype::Complex64:
    taco_not_supported_yet;
    return PochiVM::Literal<bool>(false);
    break;
  case taco::Datatype::Complex128:
    taco_not_supported_yet;
    return PochiVM::Literal<bool>(false);
    break;
  case taco::Datatype::Undefined:
    taco_ierror << "Undefined type in IR";
    return PochiVM::Literal<bool>(false);
    break;
  }
}

static int indent = 0;
struct indent_guard {
  int original_indent;
  indent_guard() : original_indent(indent) {}
  ~indent_guard() { indent = original_indent; }
};

static void debugPrint(string s) {
  bool debug = false;
  if (debug) {
    for (int i = 0; i < indent; i++) {
      std::cout << "  ";
    }
    std::cout << s << std::endl;
    ++indent;
  }
}
PochiCompiler::PochiSum PochiCompiler::visit_taco_op(taco::ir::IRNode *op) {
  indent_guard id;

  switch (op->type_info()) {
  case taco::ir::IRNodeType::Literal:
    debugPrint("Literal");
    return check_and_visit(op_cast<taco::ir::Literal *>(op));
  case taco::ir::IRNodeType::Var:
    debugPrint("Var");
    return check_and_visit(op_cast<taco::ir::Var *>(op));
  case taco::ir::IRNodeType::Neg:
    debugPrint("Neg");
    return check_and_visit(op_cast<taco::ir::Neg *>(op));
  case taco::ir::IRNodeType::Sqrt:
    debugPrint("Sqrt");
    return check_and_visit(op_cast<taco::ir::Sqrt *>(op));
  case taco::ir::IRNodeType::Add:
    debugPrint("Add");
    return check_and_visit(op_cast<taco::ir::Add *>(op));
  case taco::ir::IRNodeType::Sub:
    debugPrint("Sub");
    return check_and_visit(op_cast<taco::ir::Sub *>(op));
  case taco::ir::IRNodeType::Mul:
    debugPrint("Mul");
    return check_and_visit(op_cast<taco::ir::Mul *>(op));
  case taco::ir::IRNodeType::Div:
    debugPrint("Div");
    return check_and_visit(op_cast<taco::ir::Div *>(op));
  case taco::ir::IRNodeType::Rem:
    debugPrint("Rem");
    return check_and_visit(op_cast<taco::ir::Rem *>(op));
  case taco::ir::IRNodeType::Min:
    debugPrint("Min");
    return check_and_visit(op_cast<taco::ir::Min *>(op));
  case taco::ir::IRNodeType::Max:
    debugPrint("Max");
    return check_and_visit(op_cast<taco::ir::Max *>(op));
  case taco::ir::IRNodeType::BitAnd:
    debugPrint("BitAnd");
    return check_and_visit(op_cast<taco::ir::BitAnd *>(op));
  case taco::ir::IRNodeType::BitOr:
    debugPrint("BitOr");
    return check_and_visit(op_cast<taco::ir::BitOr *>(op));
  case taco::ir::IRNodeType::Not:
    debugPrint("Not");
    taco_not_supported_yet;
  case taco::ir::IRNodeType::Eq:
    debugPrint("Eq");
    return check_and_visit(op_cast<taco::ir::Eq *>(op));
  case taco::ir::IRNodeType::Neq:
    debugPrint("Neq");
    return check_and_visit(op_cast<taco::ir::Neq *>(op));
  case taco::ir::IRNodeType::Gt:
    debugPrint("Gt");
    return check_and_visit(op_cast<taco::ir::Gt *>(op));
  case taco::ir::IRNodeType::Lt:
    debugPrint("Lt");
    return check_and_visit(op_cast<taco::ir::Lt *>(op));
  case taco::ir::IRNodeType::Gte:
    debugPrint("Gte");
    return check_and_visit(op_cast<taco::ir::Gte *>(op));
  case taco::ir::IRNodeType::Lte:
    debugPrint("Lte");
    return check_and_visit(op_cast<taco::ir::Lte *>(op));
  case taco::ir::IRNodeType::And:
    debugPrint("And");
    return check_and_visit(op_cast<taco::ir::And *>(op));
  case taco::ir::IRNodeType::Or:
    debugPrint("Or");
    return check_and_visit(op_cast<taco::ir::Or *>(op));
  case taco::ir::IRNodeType::Cast:
    debugPrint("Cast");
    return check_and_visit(op_cast<taco::ir::Cast *>(op));
  case taco::ir::IRNodeType::Call:
    debugPrint("Call");
    return check_and_visit(op_cast<taco::ir::Call *>(op));
  case taco::ir::IRNodeType::IfThenElse:
    debugPrint("IfThenElse");
    return check_and_visit(op_cast<taco::ir::IfThenElse *>(op));
  case taco::ir::IRNodeType::Case:
    debugPrint("Case");
    return check_and_visit(op_cast<taco::ir::Case *>(op));
  case taco::ir::IRNodeType::Switch:
    debugPrint("Switch");
    return check_and_visit(op_cast<taco::ir::Switch *>(op));
  case taco::ir::IRNodeType::Load:
    debugPrint("Load");
    return check_and_visit(op_cast<taco::ir::Load *>(op));
  case taco::ir::IRNodeType::Malloc:
    debugPrint("Malloc");
    return check_and_visit(op_cast<taco::ir::Malloc *>(op));
  case taco::ir::IRNodeType::Sizeof:
    debugPrint("Sizeof");
    return check_and_visit(op_cast<taco::ir::Sizeof *>(op));
  case taco::ir::IRNodeType::Store:
    debugPrint("Store");
    return check_and_visit(op_cast<taco::ir::Store *>(op));
  case taco::ir::IRNodeType::For:
    debugPrint("For");
    return check_and_visit(op_cast<taco::ir::For *>(op));
  case taco::ir::IRNodeType::While:
    debugPrint("While");
    return check_and_visit(op_cast<taco::ir::While *>(op));
  case taco::ir::IRNodeType::Block:
    debugPrint("Block");
    return check_and_visit(op_cast<taco::ir::Block *>(op));
  case taco::ir::IRNodeType::Scope:
    debugPrint("Scope");
    return check_and_visit(op_cast<taco::ir::Scope *>(op));
  case taco::ir::IRNodeType::Function:
    debugPrint("Function");
    return check_and_visit(op_cast<taco::ir::Function *>(op));
  case taco::ir::IRNodeType::VarDecl:
    debugPrint("VarDecl");
    return check_and_visit(op_cast<taco::ir::VarDecl *>(op));
  case taco::ir::IRNodeType::VarAssign:
    debugPrint("VarAssign");
    return check_and_visit(op_cast<taco::ir::Assign *>(op));
  case taco::ir::IRNodeType::Yield:
    debugPrint("Yield");
    return check_and_visit(op_cast<taco::ir::Yield *>(op));
  case taco::ir::IRNodeType::Allocate:
    debugPrint("Allocate");
    return check_and_visit(op_cast<taco::ir::Allocate *>(op));
  case taco::ir::IRNodeType::Free:
    debugPrint("Free");
    return check_and_visit(op_cast<taco::ir::Free *>(op));
  case taco::ir::IRNodeType::Comment:
    debugPrint("Comment");
    return check_and_visit(op_cast<taco::ir::Comment *>(op));
  case taco::ir::IRNodeType::BlankLine:
    debugPrint("BlankLine");
    return check_and_visit(op_cast<taco::ir::BlankLine *>(op));
  case taco::ir::IRNodeType::Print:
    debugPrint("Print");
    return check_and_visit(op_cast<taco::ir::Print *>(op));
  case taco::ir::IRNodeType::GetProperty:
    debugPrint("GetProperty");
    return check_and_visit(op_cast<taco::ir::GetProperty *>(op));
  case taco::ir::IRNodeType::Continue:
    debugPrint("Continue");
    return check_and_visit(op_cast<taco::ir::Continue *>(op));
  case taco::ir::IRNodeType::Sort:
    debugPrint("Sort");
    return check_and_visit(op_cast<taco::ir::Sort *>(op));
  case taco::ir::IRNodeType::Break:
    debugPrint("Break");
    return check_and_visit(op_cast<taco::ir::Break *>(op));
  }
}
PochiCompiler::PochiSum
PochiCompiler::visit_taco_op(const taco::ir::IRNode *const op) {
  return visit_taco_op(const_cast<IRNode *>(op));
}
PochiCompiler::PochiSum PochiCompiler::visit_taco_op(const taco::ir::Stmt op) {
  return visit_taco_op(const_cast<IRNode *>(op.ptr));
}

PochiCompiler::PochiSum PochiCompiler::visit_taco_op(const taco::ir::Expr op) {
  return visit_taco_op(const_cast<IRNode *>(op.ptr));
}

PochiVM::AstNodeBase *
PochiCompiler::get_value_pointer(PochiCompiler::PochiSum value) {
  return std::visit(
      [&](auto &&value) -> PochiVM::AstNodeBase * {
        using ExprType = std::decay_t<decltype(value)>;
        using InnerExprType = inner_type_t<ExprType>;
        if constexpr (has_value_ptr_v<ExprType>) {
          return value.__pochivm_value_ptr;
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      value);
}

PochiVM::AstNodeBase *
PochiCompiler::get_reference_pointer(PochiCompiler::PochiSum value) {
  return std::visit(
      [&](auto &&value) -> PochiVM::AstNodeBase * {
        using ExprType = std::decay_t<decltype(value)>;
        using InnerExprType = inner_type_t<ExprType>;
        if constexpr (is_pochi_reference_v<ExprType>) {
          return value.__pochivm_ref_ptr;
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      value);
}
PochiVM::Value<void>
PochiCompiler::visit_taco_stmt(PochiCompiler::PochiSum stmt) {
  return std::visit(
      [&](auto &&stmt_to_visit) -> PochiVM::Value<void> {
        if constexpr (std::is_same_v<
                          inner_type_t<std::decay_t<decltype(stmt_to_visit)>>,
                          void>) {
          return stmt_to_visit;
        } else {
          { taco_uerror << "Invalid Type for Add"; }
          __builtin_unreachable();
        }
      },
      stmt);
}

