// GOTO Strategy:
// Insert the code where it would have been anyway.
// Have a map from name to block. Insert the code for the goto and mark position
// in the overall ret vector in a position map At the end of the function or
// something, loop through the position map and to each of the corresponding
// blocks insert everything after that position map Does this fail in case where
// we have to include other blocks in our block? Does it matter? Maybe do this
// process backwards? I don't think it does
#define IGNORE(...)
#include "taco/ir/simplify.h"
#include "compile_to_pochivt.h"
#include "callables.h"

static int countForYields(const taco::ir::For *loop) {
  struct CountYields : public IRVisitor {
    int yields = 0;

    using IRVisitor::visit;

    void visit(const Yield *op) { yields++; }
  };

  CountYields counter;
  Stmt(loop).accept(&counter);
  return counter.yields;
}
static std::vector<const GetProperty *>
sortProps(std::map<taco::ir::Expr, std::string, taco::ir::ExprCompare> map) {
  vector<const taco::ir::GetProperty *> sortedProps;

  for (auto const &p : map) {
    if (p.first.as<GetProperty>())
      sortedProps.push_back(p.first.as<GetProperty>());
  }

  // sort the properties in order to generate them in a canonical order
  sort(sortedProps.begin(), sortedProps.end(),
       [&](const GetProperty *a, const GetProperty *b) -> bool {
         // if they're different properties, sort by property
         if (a->property != b->property)
           return a->property < b->property;

         // now either the mode gives order, or index #
         if (a->mode != b->mode)
           return a->mode < b->mode;

         return a->index < b->index;
       });

  return sortedProps;
}
char *malloc_string(std::string name) {
  char *name_string = new char[name.size() + 1];
  strcpy(name_string, name.c_str());
  return name_string;
}
// TODO: Fix leak in For
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Literal *op) {
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
    silence_noret;
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
    silence_noret;
    break;
  case taco::Datatype::Float32:
    return PochiVM::Literal<float>(op->getValue<float>());
  case taco::Datatype::Float64:
    return PochiVM::Literal<double>(op->getValue<double>());
  case taco::Datatype::Complex64:
    taco_not_supported_yet;
    silence_noret;
    break;
  case taco::Datatype::Complex128:
    taco_not_supported_yet;
    silence_noret;
    break;
  case taco::Datatype::Undefined:
    taco_ierror << "Undefined type in IR";
    silence_noret;
    break;
  }
}
//
PochiVM::VariableVT PochiVTCompiler::visit_pochi(const taco::ir::Var *op) {
  return pochiVarMap.at(varMap.at(op));
  if (varNames.contains(op)) {
    return pochiVarMap.at(varNames.get(op));
  } else {
    return pochiVarMap.at(op->name);
  }
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Neg *op) {
  PochiVM::ValueVT op_val = visit_taco_op(op);
  if (op_val.HasType(PochiVM::TypeId::Get<bool>())) {
    return !(PochiVM::Value<bool>(op_val));
  } else {
    // -a = a - (a + a)
    return PochiVM::CreateArithmeticExpr(
        op_val,
        PochiVM::CreateArithmeticExpr(op_val, op_val,
                                      PochiVM::AstArithmeticExprType::ADD),
        PochiVM::AstArithmeticExprType::SUB);
  }
}
//
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Sqrt *op) {
  taco_tassert(op->type.isFloat() && op->type.getNumBits() == 64)
      << "Codegen doesn't currently support non-double sqrt";
  auto arg = visit_taco_op(op);
  return PochiVM::CallFreeFn::sqrt_double(arg);
}
PochiVM::ValueVT
PochiVTCompiler::arith_helper(const taco::ir::Expr &lhs,
                              const taco::ir::Expr &rhs,
                              PochiVM::AstArithmeticExprType exprtype) {
  return PochiVM::CreateArithmeticExpr(visit_taco_op(lhs), visit_taco_op(rhs),
                                       exprtype);
}
PochiVM::ValueVT
PochiVTCompiler::comparison_helper(const taco::ir::Expr &lhs,
                                   const taco::ir::Expr &rhs,
                                   PochiVM::AstComparisonExprType exprtype) {
  return PochiVM::CreateComparisonExpr(visit_taco_op(lhs), visit_taco_op(rhs),
                                       exprtype);
}
bool isVal(int64_t lit, const taco::ir::Expr &expr) {
  return false;
  auto ptr = expr.ptr;
  return ptr->type_info() == taco::ir::IRNodeType::Literal && expr.as<taco::ir::Literal>()->type.isInt() && 
  expr.as<taco::ir::Literal>()->getIntValue() == lit;
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Add *op) {
  if((isa<Var>(op->a) && to<Var>(op->a)->is_ptr) || (isa<GetProperty>(op->a) /*TODO: FIX THIS*/)) {
    return new PochiVM::AstPointerArithmeticExpr(visit_taco_op(op->a).__pochivm_value_ptr, 
                                            visit_taco_op(op->b).__pochivm_value_ptr, true /*isaddition*/);
  }
  return arith_helper(op->a, op->b, PochiVM::AstArithmeticExprType::ADD);
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Sub *op) {
  return arith_helper(op->a, op->b, PochiVM::AstArithmeticExprType::SUB);
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Mul *op) {
  if(isVal(1, op->a)) {
    return visit_taco_op(op->b);
  }
  if(isVal(1, op->b)) {
    return visit_taco_op(op->a);
  }
  if(isVal(0, op->a)) {
    return visit_taco_op(op->a);
  }
  if(isVal(0, op->b)) {
    return visit_taco_op(op->b);
  }
  return arith_helper(op->a, op->b, PochiVM::AstArithmeticExprType::MUL);
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Div *op) {
  return arith_helper(op->a, op->b, PochiVM::AstArithmeticExprType::DIV);
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Rem *op) {
  return arith_helper(op->a, op->b, PochiVM::AstArithmeticExprType::MOD);
}
//
PochiVM::ValueVT PochiVTCompiler::min_helper(const std::vector<Expr>& vec, int idx) {
  if(idx == vec.size() - 1) {
    return visit_taco_op(vec[idx]);
  }
  return PochiVM::CallFreeFn::min(PochiVM::StaticCast<int>(visit_taco_op(vec[idx])), min_helper(vec, idx + 1));
}
PochiVM::ValueVT PochiVTCompiler::max_helper(const std::vector<Expr>& vec, int idx) {
  if(idx == vec.size() - 1) {
    return visit_taco_op(vec[idx]);
  }
  return PochiVM::CallFreeFn::max(PochiVM::StaticCast<int>(visit_taco_op(vec[idx])), max_helper(vec, idx + 1));
}

PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Min *op) {
  return min_helper(op->operands, 0);
}
//
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Max *op) {
  return max_helper(op->operands, 0);
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::BitAnd *op) {
  auto arg1 = visit_taco_op(op->a);
  auto arg2 = visit_taco_op(op->b);
  return PochiVM::CallFreeFn::pochi_bitand(PochiVM::StaticCast<long>(arg1),
                                           PochiVM::StaticCast<long>(arg2));
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::BitOr *op) {
  auto arg1 = visit_taco_op(op->a);
  auto arg2 = visit_taco_op(op->b);
  return PochiVM::CallFreeFn::pochi_bitor(PochiVM::StaticCast<long>(arg1),
                                          PochiVM::StaticCast<long>(arg2));
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Eq *op) {
  return comparison_helper(op->a, op->b, PochiVM::AstComparisonExprType::EQUAL);
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Neq *op) {
  return comparison_helper(op->a, op->b,
                           PochiVM::AstComparisonExprType::NOT_EQUAL);
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Gt *op) {
  return comparison_helper(op->a, op->b,
                           PochiVM::AstComparisonExprType::GREATER_THAN);
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Lt *op) {
  return comparison_helper(op->a, op->b,
                           PochiVM::AstComparisonExprType::LESS_THAN);
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Gte *op) {
  return comparison_helper(op->a, op->b,
                           PochiVM::AstComparisonExprType::GREATER_EQUAL);
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Lte *op) {
  return comparison_helper(op->a, op->b,
                           PochiVM::AstComparisonExprType::LESS_EQUAL);
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::And *op) {
  auto lhs = visit_taco_op(op->a);
  auto rhs = visit_taco_op(op->b);
  return PochiVM::Value<bool>(lhs) && PochiVM::Value<bool>(rhs);
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Or *op) {
  auto lhs = visit_taco_op(op->a);
  auto rhs = visit_taco_op(op->b);
  return PochiVM::Value<bool>(lhs) || PochiVM::Value<bool>(rhs);
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Cast *op) {
  auto arg = visit_taco_op(op->a);
#define stmt(PochiType, type) return PochiVM::StaticCast<type>(arg);
  switch_over_Datatype(op->type.getKind())
#undef stmt
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::IfThenElse *op) {
  taco_iassert(op->cond.defined());
  taco_iassert(op->then.defined());
  // if(cond)
  auto cond = PochiVM::Value<bool>(visit_taco_op(op->cond));
  auto then = PochiVM::Block();
  Stmt scopedStmt = Stmt(to<Scope>(op->then)->scopedStmt);
  if (isa<Assign>(scopedStmt)) {
    then = PochiVM::Value<void>(visit_taco_op(scopedStmt));
  } else {
    then = PochiVM::Value<void>(visit_taco_op(op->then));
  }
  auto ret = PochiVM::If(cond).Then(then);
  if (op->otherwise.defined()) {
    auto otherwise = visit_taco_op(op->otherwise);
    return PochiVM::Value<void>(ret.Else(otherwise));
  } else {
    return PochiVM::Value<void>(ret);
  }
}
static PochiVM::ValueVT case_helper(std::vector<PochiVM::ValueVT> &conditions,
                                    std::vector<PochiVM::ValueVT> &bodies,
                                    int case_idx, bool alwaysMatch) {
  auto cond = PochiVM::Value<bool>(conditions[case_idx]);
  auto body = PochiVM::Value<void>(bodies[case_idx]);
  if (case_idx == bodies.size() - 1) {
    if (!alwaysMatch) {
      return PochiVM::Value<void>(PochiVM::If(cond).Then(body));
    } else {
      return body;
    }
  } else {
    return PochiVM::Value<void>(PochiVM::If(cond).Then(body).Else(
        case_helper(conditions, bodies, case_idx + 1, alwaysMatch)));
  }
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Case *op) {
  std::vector<PochiVM::ValueVT> conditions;
  std::vector<PochiVM::ValueVT> bodies;
  for (auto &clause : op->clauses) {
    conditions.push_back(visit_taco_op(clause.first));
    bodies.push_back(visit_taco_op(clause.second));
  }
  return case_helper(conditions, bodies, 0, op->alwaysMatch);
}
static PochiVM::Value<void>
switch_helper(PochiVM::ValueVT &controlExpr,
              std::vector<PochiVM::ValueVT> &conditions,
              std::vector<PochiVM::Value<void>> &bodies, int curr_case) {
  if (curr_case == conditions.size() - 1) {
    return PochiVM::If(PochiVM::CreateComparisonExpr(
                           controlExpr, conditions[curr_case],
                           PochiVM::AstComparisonExprType::EQUAL))
        .Then(bodies[curr_case]);
  } else {

    return PochiVM::If(PochiVM::CreateComparisonExpr(
                           controlExpr, conditions[curr_case],
                           PochiVM::AstComparisonExprType::EQUAL))
        .Then(bodies[curr_case])
        .Else(switch_helper(controlExpr, conditions, bodies, curr_case + 1));
  }
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Switch *op) {
  /*
  switch (op->controlExpr) {
    case switchCase[0].first:
      return switchCase[0].second;
  }
  */
  auto controlExpr = visit_taco_op(op->controlExpr);
  std::vector<PochiVM::ValueVT> conditions;
  std::vector<PochiVM::Value<void>> bodies;
  for (auto &clause : op->cases) {
    conditions.push_back(visit_taco_op(clause.first));
    bodies.push_back(visit_taco_op(clause.second));
  }
  return switch_helper(controlExpr, conditions, bodies, 0);
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Load *op) {
  auto arr = visit_taco_op(op->arr);
  auto loc = visit_taco_op(op->loc);
  return PochiVM::ReferenceVT(new PochiVM::AstPointerArithmeticExpr(
      arr.__pochivm_value_ptr, loc.__pochivm_value_ptr, true /*isAddition*/));
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Malloc *op) {
  return PochiVM::CallFreeFn::pochi_malloc(
      PochiVM::StaticCast<unsigned long>(visit_taco_op(op->size)));
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Sizeof *op) {
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
//
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Store *op) {
  auto arr = visit_taco_op(op->arr);
  auto loc = visit_taco_op(op->loc);

  return PochiVM::Assign(
      PochiVM::ReferenceVT(new PochiVM::AstPointerArithmeticExpr(
          arr.__pochivm_value_ptr, loc.__pochivm_value_ptr,
          true /*isAddition*/)),
      visit_taco_op(op->data));
}
//
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::For *op) {
  auto var = visit_var(&op->var);
  bool body_is_yield =
      op->contents.ptr->type_info() == taco::ir::IRNodeType::Yield;
  taco_iassert(typeid(var) == typeid(PochiVM::VariableVT));
  auto start = visit_taco_op(op->start);
  auto assign_stmt = body_is_yield
                         ? PochiVM::If(!goto_indicators[yields_visited++])
                               .Then(PochiVM::Assign(var, start))
                         : PochiVM::Value<void>(PochiVM::Assign(var, start));
  ;
  auto cmp_expr = PochiVM::CreateComparisonExpr(
      var, visit_taco_op(op->end), PochiVM::AstComparisonExprType::LESS_THAN);
  auto lit = op->increment.as<Literal>();
  auto body = PochiVM::Value<void>(visit_taco_op(op->contents));
  if (lit != nullptr && ((lit->type.isInt() && lit->equalsScalar(1)) ||
                         (lit->type.isUInt() && lit->equalsScalar(1)))) {
    // restore when we fix mem2reg for IncrementVar
    #if 0
    auto inc_stmt = IncrementVar(var);
    return PochiVM::For(assign_stmt, cmp_expr, inc_stmt).Do(body);
    #else
    auto one = new PochiVM::AstStaticCastExpr(
        PochiVM::Literal<long>(1).__pochivm_value_ptr, var.GetType());
    auto val = new PochiVM::AstArithmeticExpr(
        PochiVM::AstArithmeticExprType::ADD, var.__pochivm_value_ptr, one);
    auto inc_stmt = PochiVM::Assign(var, val, true);
    return PochiVM::For(assign_stmt, cmp_expr, inc_stmt).Do(body);
    #endif
  } else {
    auto inc_stmt = PochiVM::Assign(
        var,
        PochiVM::CreateArithmeticExpr(var, visit_taco_op(op->increment),
                                      PochiVM::AstArithmeticExprType::ADD));
    return PochiVM::For(assign_stmt, cmp_expr, inc_stmt).Do(body);
  }
}

//
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::While *op) {
  auto cond = PochiVM::Value<bool>(visit_taco_op(op->cond));
  auto body = PochiVM::Value<void>(visit_taco_op(op->contents));
  return PochiVM::Value<void>(PochiVM::While(cond).Do(body));
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Block *op) {
  auto ret = PochiVM::Block();
  for (const auto &stmt : op->contents) {
    // If there is a yield in a for loop insert it later
    if (stmt.ptr->type_info() == taco::ir::IRNodeType::For &&
        countForYields(stmt.as<taco::ir::For>()) > 0 && !inForYield) {
      inForYield = true;
      labelVec.push_back(PochiVM::Value<void>(visit_taco_op(stmt)));
      inForYield = false;
    } else {
      ret.Append(PochiVM::Value<void>(visit_taco_op(stmt)));
    }
  }
  return PochiVM::Value<void>(ret);
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Scope *op) {
  varNames.scope();
  auto ret = visit_taco_op(op->scopedStmt);
  varNames.unscope();
  return ret;
}
PochiVM::Value<void> PochiVTCompiler::init_params(
    const taco::ir::Function *func,
    const std::map<Expr, std::string, ExprCompare> &inputMap,
    const std::map<Expr, std::string, ExprCompare> &outputMap) {

  PochiVM::Block ret = PochiVM::Block();
  const auto returnType = func->getReturnType();
  if (returnType.second != Datatype()) {
    // void **`ctxName`;
    auto ctxVar = fn.NewVariable<void **>(malloc_string(ctxName));
    decls.Append(
        PochiVM::Declare(ctxVar, PochiVM::ReinterpretCast<void **>(params[0])));
    pochiVarMap.insert(
        {ctxName, PochiVM::VariableVT(ctxVar.__pochivm_var_ptr)});
    // char *`coordsName`
    auto coordsVar = fn.NewVariable<char *>(malloc_string(coordsName));
    decls.Append(PochiVM::Declare(coordsVar,
                                  PochiVM::ReinterpretCast<char *>(params[1])));
    pochiVarMap.insert(
        {coordsName, PochiVM::VariableVT(coordsVar.__pochivm_var_ptr)});
// `returnType.second` *`valName`
#define stmt(PochiType, type)                                                  \
  {                                                                            \
    auto valVar = fn.NewVariable<type *>(malloc_string(valName));              \
    decls.Append(PochiVM::Declare(                                             \
        valVar, PochiVM::ReinterpretCast<type *>(params[2])));                 \
    pochiVarMap.insert(                                                        \
        {valName, PochiVM::VariableVT(valVar.__pochivm_var_ptr)});             \
  }
    switch_over_Datatype(returnType.second.getKind());
#undef stmt
    // int32_t *bufCapacityName
    auto bufCapacityVar =
        fn.NewVariable<int32_t *>(malloc_string(bufCapacityName));
    decls.Append(PochiVM::Declare(
        bufCapacityVar, PochiVM::ReinterpretCast<int32_t *>(params[3])));
    pochiVarMap.insert({bufCapacityName,
                        PochiVM::VariableVT(bufCapacityVar.__pochivm_var_ptr)});
  }

  int curr_param = 0;
  bool unfoldOutput = false;
  for (size_t i = 0; i < func->outputs.size(); i++) {
    auto var = func->outputs[i].as<Var>();
    taco_iassert(var) << "Unable to convert output " << func->outputs[i]
                      << " to Var";
    if (var->is_parameter) {
      unfoldOutput = true;
      break;
    }
  }
  if (unfoldOutput) {
    for (auto prop : sortProps(outputMap)) {
      ret.Append(emitTensorProperty(outputMap.at(prop), prop, true));
    }
  }

  bool unfoldInput = false;
  for (size_t i = 0; i < func->inputs.size(); i++) {
    auto var = func->inputs[i].as<Var>();
    taco_iassert(var) << "Unable to convert output " << func->inputs[i]
                      << " to Var";
    if (var->is_parameter) {
      unfoldInput = true;
      break;
    }
  }

  if (unfoldInput) {
    for (auto prop : sortProps(inputMap)) {
      ret.Append(emitTensorProperty(inputMap.at(prop), prop, false));
    }
  }
  return ret;
}
static PochiVM::Value<bool>
or_vector(const std::vector<PochiVM::Variable<bool>> &conds, int start) {
  if (start == conds.size()) {
    return PochiVM::Literal<bool>(true);
  } else {
    return conds[start] || or_vector(conds, start + 1);
  }
}
// Yields, rather than adding to current block, add to their own special yield
// map In the main function visitor, figure out all the loop nests, conditionals
// with the blocks from the yield map The rest of the visitors should go in the
// innermost loop
//
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Function *func) {
  int numYields = countYields(func);
  emittingCoroutine = (numYields > 0);
  funcName = func->name;
  labelCount = 0;
  resetUniqueNameCounters();
  FindVars inputVarFinder(func->inputs, {}, this, &pochiVarMap, body);
  func->body.accept(&inputVarFinder);
  FindVars outputVarFinder({}, func->outputs, this, &pochiVarMap, body);
  func->body.accept(&outputVarFinder);
  // Generate Function Declaration
  // Look at shims
  body.Append(
      init_params(func, inputVarFinder.varDecls, outputVarFinder.varDecls));
  // find all the vars that are not inputs or outputs and declare them
  resetUniqueNameCounters();
  FindVars varFinder(func->inputs, func->outputs, this, &pochiVarMap, body);
  func->body.accept(&varFinder);
  varMap = varFinder.varMap;
  localVars = varFinder.localVars;
  // point each param variable to its corresponding output variable
  int curr_param = 0;
  for (auto &var : func->outputs) {
    body.Append(PochiVM::Assign(
        pochiVarMap.at(var.as<Var>()->name),
        PochiVM::ReinterpretCast<taco_tensor_t *>(params[curr_param++])));
  }
  for (auto &var : func->inputs) {
    body.Append(PochiVM::Assign(
        pochiVarMap.at(var.as<Var>()->name),
        PochiVM::ReinterpretCast<taco_tensor_t *>(params[curr_param++])));
  }
  body.Append(emitDecls(varFinder.varDecls, func->inputs, func->outputs));
  if (emittingCoroutine) {
    body.Append(
        emitContextDeclAndInit(varMap, localVars, numYields, func->name));
  }
  body.Append(visit_taco_op(taco::ir::simplify(func->body)));
  // output repack only if we allocated memory
  if (checkForAlloc(func)) {
    body.Append(emitPack(varFinder.outputProperties, func->outputs));
  }
  if (emittingCoroutine) {
    body.Append(emitCoroutineFinish(numYields, funcName));
  }

  body.Append(PochiVM::Return(PochiVM::Literal<int>(0)));
  body = PochiVM::Block(decls, body);
  indent_guard::reset();
  fn.SetBody(body);
 //cout << pochiToStr(body.GetPtr()) << endl;
  return PochiVM::Value<void>(body);
}
//
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::VarDecl *op) {
  auto var = visit_var(&op->var);
  return PochiVM::Assign(var, visit_taco_op(op->rhs));
}
//
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Assign *op) {
  // pattern match for a = a + (int)(b == c)
  // restore when we have fixed incrifeq for mem2reg
  #if 0
  if (isa<taco::ir::Add>(op->rhs)) {
    auto add = to<Add>(op->rhs);
    if (add->a == op->lhs) {
      if (isa<taco::ir::Cast>(add->b)) {
        auto cast = to<Cast>(add->b);
        if(isa<taco::ir::Eq>(cast->a)) {
          auto eq = to<Eq>(cast->a);
          if(isa<taco::ir::Var>(eq->a) && isa<taco::ir::Var>(eq->b)) {
            auto pochiL = visit_var(&eq->a);
            auto pochiR = visit_var(&eq->b);
            auto lhs = visit_var(&op->lhs);
            return PochiVM::IncrIfEq(lhs, pochiL, pochiR);
          }
        }
      }
    }
  }
  #endif
  // pattern match for a = a + 1
  // restore when incrementvar works for mem2reg
  bool shouldIncrement = false;
  if (isa<ir::Add>(op->rhs)) {
    auto add = to<Add>(op->rhs);
    if (add->a == op->lhs) {
      const Literal* lit = add->b.as<Literal>();
      if (lit != nullptr && ((lit->type.isInt()  && lit->equalsScalar(1)) ||
                              (lit->type.isUInt() && lit->equalsScalar(1)))) {
        shouldIncrement = true;
      }
    }
  }
  auto lhs = visit_var(&op->lhs);
  auto rhs = visit_taco_op(op->rhs);
  return PochiVM::Assign(lhs, rhs, shouldIncrement);
}
//
PochiVM::ValueVT PochiVTCompiler::taco_deref(std::string name) {
#define stmt(Pochitype, type)                                                  \
  return ((*(PochiVM::ReinterpretCast<type *>(                                 \
              PochiVM::ReinterpretCast<void **>(                               \
                  pochiVarMap.at(ctxName))[ctxOffsetMap[name]]))))             \
      .__pochivm_value_ptr;                                                    \
  ;
  switch_over_Datatype(ctxTypeMap.at(name).getKind());
#undef stmt
}

PochiVM::Value<void> PochiVTCompiler::taco_deref_assign(std::string name,
                                                        PochiVM::ValueVT val) {
#define stmt(Pochitype, type)                                                  \
  return PochiVM::Assign(                                                      \
      (*(PochiVM::ReinterpretCast<type *>(PochiVM::ReinterpretCast<void **>(   \
          pochiVarMap.at(ctxName))[ctxOffsetMap[name]]))),                     \
      PochiVM::StaticCast<type>(val));
  switch_over_Datatype(ctxTypeMap.at(name).getKind());
#undef stmt
}

PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Yield *op) {
  PochiVM::Block ret;
  // Potentially: set a variable called 'entering yield' or something and
  // jump to end of yield for the first time we call yield
  int stride = 0;
  for (auto &coord : op->coords) {
    stride += coord.type().getNumBytes();
  }

  int offset = 0;
  for (auto &coord : op->coords) {
    /*
    *(*coord.type())(coordsName+stride*) bufSizeName+(offset if offset > 0)
    = visit(coord);

    NOT GENERATED CODE: offset += coord.type().getNumBytes();
    */
#define stmt(PochiType, type)                                                  \
  {                                                                            \
    auto lhs = *PochiVM::ReinterpretCast<type *>(                              \
        PochiVM::ReinterpretCast<char *>(pochiVarMap.at(coordsName)) +         \
        PochiVM::Literal<int>(stride) *                                        \
            PochiVM::StaticCast<int>(pochiVarMap.at(bufSizeName)));            \
    auto rhs = visit_taco_op(coord);                                           \
    ret.Append(PochiVM::Assign(lhs, PochiVM::StaticCast<type>(rhs)));          \
  }
    switch_over_Datatype(coord.type().getKind());
#undef stmt
// valName[bufSzename] = visit(val)
#define stmt(PochiType, type)                                                  \
  {                                                                            \
    auto lhs = PochiVM::ReinterpretCast<type *>(pochiVarMap.at(                \
        valName))[PochiVM::StaticCast<int>(pochiVarMap.at(bufSizeName))];      \
    auto rhs = PochiVM::StaticCast<type>(visit_taco_op(op->val));              \
    ret.Append(PochiVM::Assign(lhs, PochiVM::StaticCast<type>(rhs)));          \
  }
    switch_over_Datatype(op->val.type().getKind());
#undef stmt

    // bufsizename ++
    ret.Append(PochiVM::Increment(PochiVM::Reference<int>(
        pochiVarMap.at(bufSizeName).__pochivm_ref_ptr)));
    // if bufsizename == bufcapacitycopyname
    indent++;
    PochiVM::Block ifBody = PochiVM::Block();
    for (auto &localVar : localVars) {
      // taco_deref(varname) = varname;
      const string varName = varMap[localVar];
      ifBody.Append(taco_deref_assign(varName, pochiVarMap.at(varName)));
    }
    ret.Append(
        PochiVM::If(
            PochiVM::StaticCast<int>(pochiVarMap.at(bufSizeName)) ==
            PochiVM::StaticCast<int>(pochiVarMap.at(bufCapacityCopyName)))
            .Then(ifBody));
    // taco_deref(statename) = labelcount;
    // return bufsizename;
    ret.Append(taco_deref_assign(stateName, PochiVM::Literal<int>(labelCount)));
    ret.Append(PochiVM::Return(PochiVM::Reference<int>(
        pochiVarMap.at(bufSizeName).__pochivm_ref_ptr)));
    return PochiVM::Value<void>(ret);
  }
  {taco_unreachable;};
  __builtin_unreachable();
}

template <typename T>
PochiVM::Value<void> allocate_helper(const taco::ir::Allocate *op,
                                     PochiVM::ReferenceVT &var,
                                     PochiVM::ValueVT &num_elements) {
  auto size =
      PochiVM::Literal<unsigned long>(static_cast<unsigned long>(sizeof(T))) *
      PochiVM::StaticCast<unsigned long>(num_elements);
  if (op->is_realloc) {
    return PochiVM::Assign(var,
                           new PochiVM::AstReinterpretCastExpr(
                               PochiVM::CallFreeFn::pochi_realloc(
                                   PochiVM::ReinterpretCast<void *>(var), size)
                                   .__pochivm_value_ptr,
                               var.GetType()));
  } else {
    // If the allocation was requested to clear the allocated memory,
    // use calloc instead of malloc.
    if (op->clear) {
      return PochiVM::Assign(
          var, PochiVM::ValueVT(new PochiVM::AstReinterpretCastExpr(
                   PochiVM::CallFreeFn::pochi_calloc(
                       PochiVM::Literal<unsigned long>(1), size)
                       .__pochivm_value_ptr,
                   var.GetType())));
    } else {
      return PochiVM::Assign(
          var, new PochiVM::AstReinterpretCastExpr(
                   PochiVM::CallFreeFn::pochi_malloc(size).__pochivm_value_ptr,
                   var.GetType()));
    }
  }
}
PochiVM::VariableVT PochiVTCompiler::visit_var(const taco::ir::Expr *op) {
  switch (op->ptr->type_info()) {
  case taco::ir::IRNodeType::GetProperty:
    debugPrint("GetProperty");
    return visit_pochi(op_cast<const taco::ir::GetProperty *>(op->ptr));
  case taco::ir::IRNodeType::Var:
    debugPrint("var");
    return visit_pochi(op_cast<const taco::ir::Var *>(op->ptr));
  default:
    {taco_ierror << "Expr type " << (int)op->ptr->type_info()
                << " cannot make a variable";}
    __builtin_unreachable();
      
  }
}
//
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Allocate *op) {
  // PochiVM::ValueVT var = visit_taco_op(op->var);
  // PochiVM::VariableVT casted_var = static_cast<PochiVM::VariableVT &>(var);
  PochiVM::VariableVT casted_var = visit_var(&op->var);
#define stmt(PochiType, type)                                                  \
  {                                                                            \
    auto num_elems = visit_taco_op(op->num_elements);                          \
    return allocate_helper<type>(op, casted_var, num_elems);                   \
  }
  switch_over_Datatype(op->var.type().getKind());
#undef stmt
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Free *op) {
  return PochiVM::CallFreeFn::pochi_free(
      PochiVM::ReinterpretCast<void *>(visit_taco_op(op->var)));
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Comment *op) {
  return PochiVM::Value<void>(PochiVM::Block());
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::BlankLine *op) {
  return PochiVM::Value<void>(PochiVM::Block());
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Print *op) {
  std::cout
      << "Will evaluate args but not print because printing doens't "
         "work right now since varargs and pochi don't work great together"
      << std::endl;
  auto ret = PochiVM::Block();
  for (const auto &e : op->params) {
    ret.Append(PochiVM::Value<void>(visit_taco_op(e)));
  }
  return PochiVM::Value<void>(ret);
}
//
PochiVM::VariableVT
PochiVTCompiler::visit_pochi(const taco::ir::GetProperty *op) {
  taco_iassert(varMap.count(op) > 0) << "Property " << Expr(op) << " of "
                                     << op->tensor << " not found in varMap";
  return pochiVarMap.at(varMap[op]);
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Continue *op) {
  return PochiVM::Value<void>(PochiVM::Continue());
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Sort *op) {
  auto base = PochiVM::ReinterpretCast<void *>(visit_taco_op(op->args[0]));
  auto num_elements =
      PochiVM::StaticCast<unsigned long>(visit_taco_op(op->args[1]));
  return PochiVM::CallFreeFn::pochi_qsort(base, num_elements);
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Break *op) {
  return PochiVM::Value<void>(PochiVM::Break());
}
PochiVM::ValueVT PochiVTCompiler::visit_pochi(const taco::ir::Call *op){
  std::vector<PochiVM::ValueVT> args;
  for (const auto &e : op->args) {
    args.push_back(visit_taco_op(e));
  }
  if(!fns.count(op->func)) {
    taco_ierror << "Function " << op->func << " not found in fns";
  }
  return fns.at(op->func)(args);
}

PochiVM::ValueVT PochiVTCompiler::visit_taco_op(taco::ir::IRNode *op) {
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
    debugPrint("GetProperty", false);
    debugPrint(op_cast<taco::ir::GetProperty *>(op)->name);
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
PochiVM::ValueVT
PochiVTCompiler::visit_taco_op(const taco::ir::IRNode *const op) {
  return visit_taco_op(const_cast<IRNode *>(op));
}
PochiVM::ValueVT PochiVTCompiler::visit_taco_op(const taco::ir::Expr &op) {
  return visit_taco_op(const_cast<IRNode *>(op.ptr));
}
PochiVM::ValueVT PochiVTCompiler::visit_taco_op(const taco::ir::Stmt &op) {
  return visit_taco_op(const_cast<IRNode *>(op.ptr));
}
PochiVM::Block PochiVTCompiler::emitDecls(
    const std::map<taco::ir::Expr, std::string, taco::ir::ExprCompare> &varMap,
    const std::vector<taco::ir::Expr> &inputs,
    const std::vector<taco::ir::Expr> &outputs) {
  auto ret = PochiVM::Block();
  unordered_set<string> propsAlreadyGenerated;

  vector<const GetProperty *> sortedProps;

  for (auto const &p : varMap) {
    if (p.first.as<GetProperty>())
      sortedProps.push_back(p.first.as<GetProperty>());
  }

  // sort the properties in order to generate them in a canonical order
  sort(sortedProps.begin(), sortedProps.end(),
       [&](const GetProperty *a, const GetProperty *b) -> bool {
         // first, use a total order of outputs,inputs
         auto a_it = find(outputs.begin(), outputs.end(), a->tensor);
         auto b_it = find(outputs.begin(), outputs.end(), b->tensor);
         auto a_pos = distance(outputs.begin(), a_it);
         auto b_pos = distance(outputs.begin(), b_it);
         if (a_it == outputs.end())
           a_pos += distance(inputs.begin(),
                             find(inputs.begin(), inputs.end(), a->tensor));
         if (b_it == outputs.end())
           b_pos += distance(inputs.begin(),
                             find(inputs.begin(), inputs.end(), b->tensor));

         // if total order is same, have to do more, otherwise we know
         // our answer
         if (a_pos != b_pos)
           return a_pos < b_pos;

         // if they're different properties, sort by property
         if (a->property != b->property)
           return a->property < b->property;

         // now either the mode gives order, or index #
         if (a->mode != b->mode)
           return a->mode < b->mode;

         return a->index < b->index;
       });

  for (auto prop : sortedProps) {
    bool isOutputProp =
        (find(outputs.begin(), outputs.end(), prop->tensor) != outputs.end());

    auto var = prop->tensor.as<Var>();
    if (var->is_parameter) {
      if (isOutputProp) {
        ret.Append(emitTensorProperty(varMap.at(prop), prop, false));
      } else {
        break;
      }
    } else {
      ret.Append(unpackTensorProperty(varMap.at(prop), prop, isOutputProp));
    }
    propsAlreadyGenerated.insert(varMap.at(prop));
  }

  return ret;
}
PochiVM::Value<void> PochiVTCompiler::unpackTensorProperty(
    std::string varname, const taco::ir::GetProperty *op, bool is_output_prop) {
  auto var = pochiVarMap.at(varname);
  PochiVM::Block ret = PochiVM::Block();
  auto tensor = op->tensor.as<Var>();
  auto tensorvar =
      PochiVM::Variable<taco_tensor_t *>(pochiVarMap.at(tensor->name));
  if (op->property == TensorProperty::Values) {
    // for the values, it's in the last slot
    // type *varname = (tensor->type *) tensor_name->vals
#define stmt(kind, type)                                                       \
  {                                                                            \
    auto casted_var = PochiVM::Variable<type *>(var);                          \
    return PochiVM::Assign(                                                    \
        casted_var, PochiVM::ReinterpretCast<type *>(tensorvar->vals()));      \
  }
    switch_over_Datatype(op->tensor.type().getKind())
#undef stmt
  } else if (op->property == TensorProperty::ValuesSize) {
    return PochiVM::Assign(var, tensorvar->vals_size());
  }

  string tp;

  // for a Dense level, nnz is an int
  // for a Fixed level, ptr is an int
  // all others are int*
  if (op->property == TensorProperty::Dimension) {
    return PochiVM::Assign(
        var, PochiVM::StaticCast<int>(
                 tensorvar->dimensions()[PochiVM::Literal<int>(op->mode)]));
  } else {
    taco_iassert(op->property == TensorProperty::Indices);
    tp = "int*";
    auto nm = op->index;
    return PochiVM::Assign(var, PochiVM::ReinterpretCast<int *>(
                                    tensorvar->indices()[op->mode][nm]));
  }
}
PochiVM::Value<void> PochiVTCompiler::emitTensorProperty(
    std::string varname, const taco::ir::GetProperty *op, bool is_ptr) {
  if (is_ptr) {
    varname += "_ptr";
  }
  return pochiVarMap.at(varname);
}
PochiVM::Value<void> PochiVTCompiler::emitPack(
    const std::map<
        std::tuple<taco::ir::Expr, taco::ir::TensorProperty, int, int>,
        std::string> &outputProperties,
    std::vector<taco::ir::Expr> outputs) {
  PochiVM::Block ret = PochiVM::Block();
  vector<tuple<Expr, TensorProperty, int, int>> sortedProps;

  for (auto &prop : outputProperties) {
    sortedProps.push_back(prop.first);
  }
  sort(sortedProps.begin(), sortedProps.end(),
       [&](const tuple<Expr, TensorProperty, int, int> &a,
           const tuple<Expr, TensorProperty, int, int> &b) -> bool {
         // first, use a total order of outputs,inputs
         auto a_it = find(outputs.begin(), outputs.end(), get<0>(a));
         auto b_it = find(outputs.begin(), outputs.end(), get<0>(b));
         auto a_pos = distance(outputs.begin(), a_it);
         auto b_pos = distance(outputs.begin(), b_it);

         // if total order is same, have to do more, otherwise we know
         // our answer
         if (a_pos != b_pos)
           return a_pos < b_pos;

         // if they're different properties, sort by property
         if (get<1>(a) != get<1>(b))
           return get<1>(a) < get<1>(b);

         // now either the mode gives order, or index #
         if (get<2>(a) != get<2>(b))
           return get<2>(a) < get<2>(b);

         return get<3>(a) < get<3>(b);
       });

  for (auto prop : sortedProps) {
    auto var = get<0>(prop).as<Var>();
    if (var->is_parameter) {
      ret.Append(pointTensorProperty(outputProperties.at(prop)));
    } else {
      ret.Append(packTensorProperty(outputProperties.at(prop), get<0>(prop),
                                    get<1>(prop), get<2>(prop), get<3>(prop)));
    }
  }
  return ret;
}
PochiVM::Value<void> PochiVTCompiler::pointTensorProperty(std::string varname) {
  return PochiVM::Assign(*pochiVarMap.at(varname + "_ptr"),
                         pochiVarMap.at(varname));
}
PochiVM::Value<void> PochiVTCompiler::packTensorProperty(
    std::string varname, const taco::ir::Expr &tnsr,
    const taco::ir::TensorProperty &property, int mode, int index) {
  auto tensor = tnsr.as<Var>();
  auto tensor_var = pochiVarMap.at(tensor->name);
  taco_iassert(typeid(tensor_var) == typeid(PochiVM::VariableVT));
  auto casted_tensor_var = PochiVM::Variable<taco_tensor_t *>(tensor_var);
  if (property == TensorProperty::Values) {
    return PochiVM::Assign(
        casted_tensor_var->vals(),
        PochiVM::ReinterpretCast<uint8_t *>(pochiVarMap.at(varname)));
  } else if (property == TensorProperty::ValuesSize) {
    return PochiVM::Assign(
        PochiVM::ReferenceVT(casted_tensor_var->vals_size().__pochivm_ref_ptr),
        pochiVarMap.at(varname));
  }

  // for a Dense level, nnz is an int
  // for a Fixed level, ptr is an int
  // all others are int*
  if (property == TensorProperty::Dimension) {
    return PochiVM::Block();
  } else {
    taco_iassert(property == TensorProperty::Indices);
    auto nm = index;
    // tensor->indices[mode][nm] = (uint8_t*)(varname)
    return PochiVM::Assign(
        casted_tensor_var
            ->indices()[PochiVM::Literal<int>(mode)][PochiVM::Literal<int>(nm)],
        PochiVM::ReinterpretCast<uint8_t *>(pochiVarMap.at(varname)));
  }
}
void PochiVTCompiler::compile(taco::ir::Stmt stmt, bool isFirst = false) {
  visit_taco_op(stmt);
}
PochiVM::Value<void> PochiVTCompiler::emitContextDeclAndInit(
    std::map<Expr, std::string, ExprCompare> varMap,
    std::vector<Expr> localVars, int labels, std::string funcName) {
  auto ret = PochiVM::Block();

  for (int i = 0; i < labels; ++i) {
    goto_indicators.push_back(fn.NewVariable<bool>(
        malloc_string(labelPrefix + funcName + std::to_string(i))));
    decls.Append(PochiVM::Declare(goto_indicators[goto_indicators.size() - 1],
                                  PochiVM::Literal<bool>(false)));
  }

  int ctxOffset = 0;
  ctxOffsetMap[sizeName] = ctxOffset++;
  ctxTypeMap[sizeName] = taco::Datatype(taco::Datatype::Int32);
  ctxOffsetMap[stateName] = ctxOffset++;
  ctxTypeMap[stateName] = taco::Datatype(taco::Datatype::Int32);
  for (auto &var : localVars) {
    ctxOffsetMap[varMap[var]] = ctxOffset++;
    ctxTypeMap[varMap[var]] = var.type();
  }

  pochiVarMap.insert(
      {bufSizeName,
       fn.NewVariable<int>(malloc_string(bufSizeName)).__pochivm_var_ptr});
  decls.Append(
      PochiVM::Declare(PochiVM::Variable<int>(pochiVarMap.at(bufSizeName)),
                       PochiVM::Literal<int>(0)));
  ret.Append(PochiVM::Assign(pochiVarMap.at(bufSizeName),
                             PochiVM::Literal<int32_t>(0)));
  pochiVarMap.insert({bufCapacityCopyName,
                      fn.NewVariable<int>(malloc_string(bufCapacityCopyName))
                          .__pochivm_var_ptr});
  decls.Append(PochiVM::Declare(
      PochiVM::Variable<int>(pochiVarMap.at(bufCapacityCopyName)),
      PochiVM::Literal<int>(0)));
  ret.Append(PochiVM::Assign(pochiVarMap.at(bufCapacityCopyName),
                             PochiVM::Literal<int32_t>(0)));
  ret.Append(PochiVM::Assign(pochiVarMap.at(bufCapacityCopyName),
                             *pochiVarMap.at(bufCapacityName)));
  PochiVM::Block ifBlock = PochiVM::Block();
  for (auto &localVar : localVars) {
    const string varName = varMap[localVar];
    ifBlock.Append(
        PochiVM::Assign(pochiVarMap.at(varName), taco_deref(varName)));

  }
  PochiVM::Block elseBlock = PochiVM::Block();
  elseBlock.Append(PochiVM::Assign(
      pochiVarMap.at(ctxName),
      PochiVM::ReinterpretCast<void **>(PochiVM::CallFreeFn::pochi_malloc(
          PochiVM::Literal<unsigned long>(ctxOffset * sizeof(void *))))));
  for (auto &var : localVars) {
    elseBlock.Append(PochiVM::Assign(
        PochiVM::ReinterpretCast<void **>(
            pochiVarMap.at(ctxName))[ctxOffsetMap[varMap[var]]],
        PochiVM::CallFreeFn::pochi_malloc(
            PochiVM::Literal<unsigned long>(var.type().getNumBytes()))));
  }
  elseBlock.Append(taco_deref_assign(
      sizeName, PochiVM::Literal<unsigned long>(sizeof(void *) * ctxOffset)));
  ret.Append(PochiVM::If(PochiVM::ReinterpretCast<uint64_t>(pochiVarMap.at(
                             ctxName)) != PochiVM::Literal<uint64_t>(static_cast<uint64_t>(NULL)))
                 .Then(ifBlock)
                 .Else(elseBlock));
  /*
  Options:
  An array of void *'s that are intiialized here or array of std::any
  struct ctxClassName {
    int32_t sizeName;
    int32_t stateName;

    localVars[0].type localVars[0].name;
    localVars[1].type localVar[1].name;
    ...
  };
    localVars[0].type localVars[0].name;
    localVars[1].type localVar[1].name;
    ...

    int32_t bufSizeName = 0;
    int32_t bufCapacityCopyName = *bufCapacityName;
    if(*ctxName) {
      localVars[0].name = TACO_DEREF(localVars[0].name);
      localVars[1].name = TACO_DEREF(localVars[1].name);
      switch(TACO_DEREF(stateName)) {
        case 0:
          goto labelPrefix + funcName + 0;
        case 1:
          goto labelPrefix + funcName + 1;
        ...
      }
    }
    else {
      printAlloc(*ctxName, sizeof(ctxClassName))
      TACO_DEREF(sizeName) = sizeof(ctxClassName);
    }
  */
  return ret;
}
PochiVM::Value<void>
PochiVTCompiler::emitCoroutineFinish(int numYields, std::string funcName) {
  return PochiVM::Block(
      PochiVM::If(PochiVM::CreateComparisonExpr(
                      pochiVarMap.at(bufSizeName), PochiVM::Literal<int32_t>(0),
                      PochiVM::AstComparisonExprType::LESS_THAN))
          .Then(PochiVM::Block(
              taco_deref_assign(stateName,
                                PochiVM::Literal<int32_t>(numYields)),
              PochiVM::Return(
                  PochiVM::StaticCast<int32_t>(pochiVarMap.at(bufSizeName))))),
      PochiVM::CallFreeFn::pochi_free(
          *PochiVM::ReinterpretCast<void **>(pochiVarMap.at(ctxName)))

  );
  /*
  if(bufSizeName > 0) {
    TACO_DEREF(stateName) = numYields;
    return bufSizeName;
  }
  labelPrefix + funcName + numYields:
    #warning MUST ALSO SET ctxalloced to false
    free(*ctxName);
    *ctxName = NULL;
  */
}
