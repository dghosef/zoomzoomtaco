#include "debug.h"
#include <iostream>
#include <string>
using namespace std;
using namespace PochiVM;
void debugPrint(string s, bool newline) {
  static bool lastnewline = newline;
  bool debug = false;
  if (debug) {
    for (int i = 0; i < indent_guard::indent && lastnewline; i++) {
      std::cout << "  ";
    }
    if (!lastnewline) {
      std::cout << " ";
    }
    std::cout << s;
    if (newline)
      std::cout << std::endl;
    else
      std::cout << std::flush;
  }
  lastnewline = newline;
}

template <typename T, typename U> auto safe_cast(U val) -> T {
  T ret = dynamic_cast<T>(val);
  assert(ret);
  return ret;
}

std::function<void(const PochiVM::AstNodeBase *stmt)>
pochiEmitter(std::stringstream &ss) {
  return [&](const PochiVM::AstNodeBase *stmt) { ss << pochiToStr(stmt); };
}
static string childToString(string begin, const AstNodeBase *node, string end, int n = 0) {
  stringstream ss;
  ss << begin;
  const_cast<AstNodeBase *>(node)->ForEachChildren([&](const AstNodeBase *child) {
    if(n-- == 0) {
      ss << pochiToStr(child);
    }
  });
  ss << end;
  return ss.str();
}
static string doIndent() {
  stringstream ret;
  for (int i = 0; i < indent_guard::indent; i++) {
    ret << "  ";
  }
  return ret.str();
}
std::string pochiToStr(const AstNodeBase *stmt) {
  std::stringstream ret;
  switch (int(stmt->GetAstNodeType())) {
  case AstNodeType::AstIncrIfEqStatement: return "IncrIfEq";
  case AstNodeType::AstIncrStatement: return "Incr";
  case AstNodeType::AstArithmeticExpr: {
    auto expr = safe_cast<const AstArithmeticExpr *>(stmt);
    ret << pochiToStr(expr->m_lhs) << " ";
    switch (expr->m_op) {
    case AstArithmeticExprType::ADD:
      ret << "+";
      break;
    case AstArithmeticExprType::SUB:
      ret << "-";
      break;
    case AstArithmeticExprType::MUL:
      ret << "*";
      break;
    case AstArithmeticExprType::DIV:
      ret << "/";
      break;
    case AstArithmeticExprType::MOD:
      ret << "%";
      break;
    default:
      assert(false);
    }
    ret << " " << pochiToStr(expr->m_rhs);
    break;
  }
  case AstNodeType::AstComparisonExpr: {
    auto expr = safe_cast<const AstComparisonExpr *>(stmt);
    ret << pochiToStr(expr->m_lhs) << " ";
    switch (expr->m_op) {
    case AstComparisonExprType::EQUAL:
      ret << "==";
      break;
    case AstComparisonExprType::NOT_EQUAL:
      ret << "!=";
      break;
    case AstComparisonExprType::LESS_THAN:
      ret << "<";
      break;
    case AstComparisonExprType::LESS_EQUAL:
      ret << "<=";
      break;
    case AstComparisonExprType::GREATER_THAN:
      ret << ">";
      break;
    case AstComparisonExprType::GREATER_EQUAL:
      ret << ">=";
      break;
    default:
      assert(false);
    }
    ret << " " << pochiToStr(expr->m_rhs);
    break;
  }
  case AstNodeType::AstStaticCastExpr: {
    ret << childToString("StaticCast<" + stmt->GetTypeId().Print() + ">(", stmt,
                         ")");
    break;
  }
  case AstNodeType::AstReinterpretCastExpr: {
    auto expr = safe_cast<const AstReinterpretCastExpr *>(stmt);
    ret << childToString("ReinterpretCast<" + expr->m_operand->GetTypeId().Print() + "->" + stmt->GetTypeId().Print() + ">(",
                         stmt, ")");
    break;
  }
  case AstNodeType::AstDereferenceExpr: {
    ret << childToString("*(", stmt, ")");
    break;
  }
  case AstNodeType::AstLiteralExpr: {
    ret << "Literal("
        << const_cast<AstLiteralExpr *>(safe_cast<const AstLiteralExpr *>(stmt))
               ->GetAsU64() << ")";
    break;
  }
  case AstNodeType::AstAssignExpr: {
    auto expr = safe_cast<const AstAssignExpr *>(stmt);
    ret << "(" << pochiToStr(expr->GetDst()) << ")"
        << " = " << pochiToStr(expr->GetSrc());
    break;
  }
  case AstNodeType::AstNullptrExpr: {
    ret << "Nullptr";
    break;
  }
  case AstNodeType::AstTrashPtrExpr: {
    ret << "AstTrashPtrExpr";
    throw std::runtime_error("unimplemented");
    break;
  }
  case AstNodeType::AstVariable: {
    auto expr = safe_cast<const AstVariable *>(stmt);
    ret << "Var(" << expr->GetVarNameNoSuffix() << expr->GetVarSuffix() << ")";
    break;
  }
  case AstNodeType::AstDeclareVariable: {
    auto expr = safe_cast<const AstDeclareVariable *>(stmt);
    ret << "Decl(" << pochiToStr(expr->m_variable);
    if (expr->m_assignExpr != nullptr) {
      ret << ", " << pochiToStr(expr->m_assignExpr);
    }
    ret << ")";
    break;
  }
  case AstNodeType::AstDereferenceVariableExpr: {
    ret << childToString("*(", stmt, ")");
    break;
  }
  case AstNodeType::AstBlock: {
    ret << "Block" << std::endl;
    indent_guard ig;
    const_cast<AstNodeBase *>(stmt)->ForEachChildren(
        [&](const AstNodeBase *child) {
          ret << doIndent() << pochiToStr(child) << std::endl;
        });
    break;
  }
  case AstNodeType::AstScope: {
    ret << "Scope" << std::endl;
    indent_guard ig;
    const_cast<AstNodeBase *>(stmt)->ForEachChildren(
        [&](const AstNodeBase *child) {
          ret << doIndent() << pochiToStr(child) << std::endl;
        });
    break;
  }
  case AstNodeType::AstIfStatement: {
    auto expr = safe_cast<const AstIfStatement *>(stmt);
    ret << "If" << std::endl;
    indent_guard ig;
    ret << doIndent() << "Cond: " << std::endl;
    {
      indent_guard ig2;
      ret << doIndent() << pochiToStr(expr->GetCondClause()) << std::endl;
    }
    ret << doIndent() << "Then: " << std::endl;
    {
      indent_guard ig2;
      ret << doIndent() << pochiToStr(expr->GetThenClause()) << std::endl;
    }
    if(expr->GetElseClause() != nullptr) {
      ret << doIndent() << "Else: " << std::endl;
      {
        indent_guard ig2;
        ret << doIndent() << pochiToStr(expr->GetElseClause()) << std::endl;
      }
    }
    break;
  }
  case AstNodeType::AstWhileLoop: {
    auto expr = safe_cast<const AstWhileLoop *>(stmt);
    ret << "While" << std::endl;
    indent_guard ig;
    ret << doIndent() << "Cond: " << std::endl;
    {
      indent_guard ig2;
      ret << doIndent() << childToString("(", expr, ")", 0)
          << std::endl;
    }
    ret << doIndent() << "Then: " << std::endl;
    {
      indent_guard ig2;
      ret << doIndent() << childToString("(", expr, ")", 1)
          << std::endl;
    }
    break;
  }
  case AstNodeType::AstForLoop: {
    ret << "For" << std::endl;
    indent_guard ig;
    ret << doIndent() << "Init: " << std::endl;
    {
      indent_guard ig2;
      ret << doIndent() << childToString("", stmt, "", 0)
          << std::endl;
    }
    ret << doIndent() << "Cond: " << std::endl;
    {
      indent_guard ig2;
      ret << doIndent() << childToString("", stmt, "", 1)
          << std::endl;
    }
    ret << doIndent() << "Increment: " << std::endl;
    {
      indent_guard ig2;
      ret << doIndent() << childToString("", stmt, "", 3)
          << std::endl;
    }
    ret << doIndent() << "Body: " << std::endl;
    {
      indent_guard ig2;
      ret << doIndent() << childToString("", stmt, "", 2)
          << std::endl;
    }
    break;
  }
  case AstNodeType::AstBreakOrContinueStmt: {
    auto expr = safe_cast<const AstBreakOrContinueStmt *>(stmt);
    ret << (expr->IsBreakStatement() ? "Break" : "Continue");
    break;
  }
  case AstNodeType::AstGotoStmt: {
    auto expr = safe_cast<const AstBreakOrContinueStmt *>(stmt);
    ret << ("GOTO");
    break;
  }
  case AstNodeType::AstLabelStmt: {
    auto expr = safe_cast<const AstBreakOrContinueStmt *>(stmt);
    ret << ("Label");
    break;
  }
  case AstNodeType::AstCallExpr: {
    auto expr = const_cast<AstCallExpr *>(safe_cast<const AstCallExpr *>(stmt));
    ret << "call(";
    ret << expr->GetFnName();
    if(expr->IsCppFunction()) {
      ret << expr->GetCppFunctionMetadata()->m_bitcodeData->m_symbolName;
    }
    expr->ForEachChildren([&](const AstNodeBase *child) {
      ret << ", " << pochiToStr(child);
    });
    ret << ")";
    break;
  }
  case AstNodeType::AstReturnStmt: {
    auto expr = const_cast<AstReturnStmt *>(safe_cast<const AstReturnStmt *>(stmt));
    ret << "return ";
    ret << pochiToStr(expr->m_retVal);
    break;
  }
  case AstNodeType::AstLogicalAndOrExpr: {
    auto expr = const_cast<AstLogicalAndOrExpr *>(
        safe_cast<const AstLogicalAndOrExpr *>(stmt));
    ret << pochiToStr(expr->m_lhs);
    ret << (expr->m_isAnd ? " && " : " || ");
    ret << pochiToStr(expr->m_rhs);
    break;
  }
  case AstNodeType::AstLogicalNotExpr: {
    auto expr = const_cast<AstLogicalNotExpr *>(
        safe_cast<const AstLogicalNotExpr *>(stmt));
    ret << "~" << pochiToStr(expr->m_op);
    break;
  }
  case AstNodeType::AstThrowStmt: {
    auto expr = const_cast<AstThrowStmt *>(safe_cast<const AstThrowStmt *>(stmt));
    ret << "throw " << pochiToStr(expr->m_operand);
    break;
  }
  case AstNodeType::AstRvalueToConstPrimitiveRefExpr: {
    ret << "AstRvalueToConstPrimitiveRefExpr";
    throw std::runtime_error("unimplemented");
    break;
  }
  case AstNodeType::AstExceptionAddressPlaceholder: {
    ret << "AstExceptionAddressPlaceholder";
    throw std::runtime_error("unimplemented");
    break;
  }
  case AstNodeType::AstPointerArithmeticExpr: {
    auto expr = const_cast<AstPointerArithmeticExpr *>(
        safe_cast<const AstPointerArithmeticExpr *>(stmt));
    ret << pochiToStr(expr->m_base) << "+" << pochiToStr(expr->m_index);
    break;
  }
  case AstNodeType::AstGeneratedFunctionPointerExpr: {
    ret << "AstGeneratedFunctionPointerExpr";
    throw std::runtime_error("unimplemented");
    break;
  }
  default:
    throw "Unknown AstNodeType";
    break;
  }
  return ret.str();
}