#pragma once
#include "pochivm.h"

// #define NO_POCHI_BACKEND
#ifdef NO_POCHI_BACKEND
#define DONT_RECOMPILE
#endif
#include <cstdint>
#include <cstring>
#include <typeinfo>
#include <variant>

#include "codegen.h"
#include "pochi_templates.h"
#include "pochivm.h"
#include "pochivm/ast_type_helper.h"
#include "runtime/pochivm_runtime_headers.h"
#include "taco/ir/ir.h"

#include "debug.h"
#include "macros.h"
#include "taco/ir/ir_visitor.h"
#include "taco/util/collections.h"
#include "taco/util/strings.h"
using namespace taco;
using namespace taco::ir;
using namespace std;

#ifdef UNSAFE
#define op_cast static_cast
#else
#define op_cast dynamic_cast
#endif

char *malloc_string(std::string name);
class PochiVTCompiler : taco::ir::CodeGen {
public:
  int yields_visited = 0;
  const std::string ctxName = "__ctx__";
  const std::string coordsName = "__coords__";
  const std::string bufCapacityName = "__bufcap__";
  const std::string valName = "__val__";
  const std::string ctxClassName = "___context___";
  const std::string sizeName = "size";
  const std::string stateName = "state";
  const std::string bufSizeName = "__bufsize__";
  const std::string bufCapacityCopyName = "__bufcapcopy__";
  const std::string labelPrefix = "resume_";
  bool inForYield = false;
  std::vector<PochiVM::Variable<bool>> goto_indicators;
  std::map<std::string, PochiVM::VariableVT> pochiVarMap;
  using fnType = int (*)(void **);
  std::tuple<PochiVM::Function, PochiVM::Variable<void **>> fnTuple;
  PochiVM::Function fn;
  PochiVM::Variable<void **> params;
  PochiVM::Block body;
  PochiVM::Block decls;
  PochiVTCompiler(std::string fnName)
      : fnTuple(PochiVM::NewFunction<fnType>(fnName)), fn(std::get<0>(fnTuple)),
        params(std::get<1>(fnTuple)), body(PochiVM::Block()),
        decls(PochiVM::Block()), taco::ir::CodeGen(std::cout,
                                                   CodeGen::CodeGenType::C){};
  std::map<std::string, int> ctxOffsetMap;
  std::map<std::string, taco::Datatype> ctxTypeMap;
  // Used to emulate the gotos that taco produces since pochi doesn't support
  // them
  std::vector<PochiVM::Value<void>> labelVec;
  PochiVM::ValueVT arith_helper(const taco::ir::Expr &lhs,
                                const taco::ir::Expr &rhs,
                                PochiVM::AstArithmeticExprType exprtype);
  PochiVM::ValueVT comparison_helper(const taco::ir::Expr &lhs,
                                     const taco::ir::Expr &rhs,
                                     PochiVM::AstComparisonExprType exprtype);
  PochiVM::ValueVT visit_pochi(const taco::ir::Literal *op);
  PochiVM::VariableVT visit_pochi(const taco::ir::Var *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Neg *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Sqrt *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Add *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Sub *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Mul *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Div *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Rem *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Min *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Max *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::BitAnd *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::BitOr *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Eq *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Neq *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Gt *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Lt *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Gte *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Lte *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::And *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Or *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Cast *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Call *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::IfThenElse *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Case *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Switch *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Load *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Malloc *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Sizeof *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Store *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::For *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::While *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Block *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Scope *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Function *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::VarDecl *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Assign *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Yield *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Allocate *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Free *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Comment *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::BlankLine *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Print *op);
  PochiVM::VariableVT visit_pochi(const taco::ir::GetProperty *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Continue *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Sort *op);
  PochiVM::ValueVT visit_pochi(const taco::ir::Break *op);
  // Check to make sure op is not nullptr and then visit
  template <typename T> PochiVM::ValueVT check_and_visit(const T op) {
#ifndef UNSAFE
    taco_iassert(op != nullptr);
#endif
    return visit_pochi(op);
  }
  class FindVars;

  PochiVM::ValueVT visit_taco_op(taco::ir::IRNode *op);
  PochiVM::ValueVT visit_taco_op(const taco::ir::IRNode *const op);
  PochiVM::ValueVT visit_taco_op(const taco::ir::Expr &op);
  PochiVM::ValueVT visit_taco_op(const taco::ir::Stmt &op);
  PochiVM::VariableVT visit_var(const taco::ir::Expr *op);
  std::string funcName;
  int labelCount;
  bool emittingCoroutine;
  std::map<taco::ir::Expr, std::string, taco::ir::ExprCompare> varMap;
  std::vector<taco::ir::Expr> localVars;
  PochiVM::Block emitDecls(const std::map<taco::ir::Expr, std::string,
                                          taco::ir::ExprCompare> &varMap,
                           const std::vector<taco::ir::Expr> &inputs,
                           const std::vector<taco::ir::Expr> &outputs);
  PochiVM::Value<void> unpackTensorProperty(std::string varname,
                                            const taco::ir::GetProperty *op,
                                            bool is_output_prop);
  PochiVM::Value<void> emitTensorProperty(std::string varname,
                                          const taco::ir::GetProperty *op,
                                          bool is_ptr);
  PochiVM::Value<void>
  emitPack(const std::map<
               std::tuple<taco::ir::Expr, taco::ir::TensorProperty, int, int>,
               std::string> &outputProperties,
           std::vector<taco::ir::Expr> outputs);
  PochiVM::Value<void> pointTensorProperty(std::string varname);
  PochiVM::Value<void>
  packTensorProperty(std::string varname, const taco::ir::Expr &tnsr,
                     const taco::ir::TensorProperty &property, int mode,
                     int index);
  void compile(taco::ir::Stmt stmt, bool isFirst);
  PochiVM::Value<void>
  emitContextDeclAndInit(std::map<Expr, std::string, ExprCompare> varMap,
                         std::vector<Expr> localVars, int labels,
                         std::string funcName);
  PochiVM::Value<void> emitCoroutineFinish(int numYields, std::string funcName);
  PochiVM::ValueVT taco_deref(std::string name);
  PochiVM::Value<void> taco_deref_assign(std::string name,
                                         PochiVM::ValueVT val);
  PochiVM::Value<void>
  init_params(const taco::ir::Function *func,
              const std::map<Expr, std::string, ExprCompare> &inputMap,
              const std::map<Expr, std::string, ExprCompare> &outputMap);
};
// find variables for generating declarations
// generates a single var for each GetProperty
class PochiVTCompiler::FindVars : public IRVisitor {
public:
  map<Expr, string, ExprCompare> varMap;
  map<string, PochiVM::VariableVT> *pochiVarMap;

  // the variables for which we need to add declarations
  map<Expr, string, ExprCompare> varDecls;

  vector<Expr> localVars;

  // this maps from tensor, property, mode, index to the unique var
  map<tuple<Expr, TensorProperty, int, int>, string> canonicalPropertyVar;

  // this is for convenience, recording just the properties unpacked
  // from the output tensor so we can re-save them at the end
  map<tuple<Expr, TensorProperty, int, int>, string> outputProperties;

  // TODO: should replace this with an unordered set
  vector<Expr> outputTensors;
  vector<Expr> inputTensors;

  PochiVTCompiler *codeGen;

  PochiVM::Block body;
  template <typename T> void insert_var(std::string name) {
    PochiVM::Variable<T> pochiVar =
        codeGen->fn.NewVariable<T>(malloc_string(name));
    codeGen->decls.Append(PochiVM::Declare(pochiVar));

    pochiVarMap->insert(
        {name, PochiVM::VariableVT(pochiVar.__pochivm_var_ptr)});
  }
  void insert_param(const Var *var) {
// FIX TYPES DOWN HERE
#define stmt(PochiType, type)                                                  \
  if (var->is_tensor)                                                          \
    insert_var<taco_tensor_t *>(var->name);                                    \
  else                                                                         \
    var->is_ptr ? insert_var<type *>(var->name) : insert_var<type>(var->name);
    switch_over_Datatype(var->type.getKind());
#undef stmt
  }

  // copy inputs and outputs into the map
  FindVars(vector<Expr> inputs, vector<Expr> outputs, PochiVTCompiler *codeGen,
           map<string, PochiVM::VariableVT> *pochiVarMap, PochiVM::Block body)
      : codeGen(codeGen), pochiVarMap(pochiVarMap), body(body) {
    for (auto v : inputs) {
      auto var = v.as<Var>();
      taco_iassert(var) << "Inputs must be vars in codegen";
      taco_iassert(varMap.count(var) == 0)
          << "Duplicate input found in codegen";
      inputTensors.push_back(v);
      insert_param(var);
    }
    for (auto v : outputs) {
      auto var = v.as<Var>();
      taco_iassert(var) << "Outputs must be vars in codegen";
      taco_iassert(varMap.count(var) == 0)
          << "Duplicate output found in codegen";
      outputTensors.push_back(v);
      insert_param(var);
    }
  }

protected:
  void insert_var(const Var *var, string name) {
    // FIX TYPES DOWN HERE
    varMap[var] = name;
#define stmt(Kind, type)                                                       \
  if (var->is_ptr)                                                             \
    insert_var<type *>(name);                                                  \
  else                                                                         \
    insert_var<type>(name);

    switch_over_Datatype(var->type.getKind())
#undef stmt
  }

  // Insert the pointer version of the variable
  void insert_var(const GetProperty *op, string name) {
    // FIX TYPES DOWN HERE
    varMap[op] = name;
    if (op->property == TensorProperty::Values) {
#define stmt(Kind, type) insert_var<type *>(name);

      switch_over_Datatype(op->type.getKind())
#undef stmt
    } else if (op->property == TensorProperty::ValuesSize ||
               op->property == TensorProperty::Dimension) {
      insert_var<int>(name);
    } else {
      taco_iassert(op->property == TensorProperty::Indices);
      insert_var<int *>(name);
    }
  }

  void insert_var(const Var *var) {
    // FIX TYPES DOWN HERE
    insert_var(var, var->name);
  }

  using IRVisitor::visit;

  virtual void visit(const Var *op) {
    if (varMap.count(op) == 0) {
      insert_var(op, op->is_ptr ? op->name : codeGen->genUniqueName(op->name));
    }
  }

  virtual void visit(const VarDecl *op) {
    if (!util::contains(localVars, op->var)) {
      localVars.push_back(op->var);
    }
    op->var.accept(this);
    op->rhs.accept(this);
  }

  virtual void visit(const For *op) {
    if (!util::contains(localVars, op->var)) {
      localVars.push_back(op->var);
    }
    op->var.accept(this);
    op->start.accept(this);
    op->end.accept(this);
    op->increment.accept(this);
    op->contents.accept(this);
  }

  virtual void visit(const GetProperty *op) {
    if (!util::contains(inputTensors, op->tensor) &&
        !util::contains(outputTensors, op->tensor)) {
      // Don't create header unpacking code for temporaries
      return;
    }

    if (varMap.count(op) == 0) {
      auto key = tuple<Expr, TensorProperty, int, int>(
          op->tensor, op->property, (size_t)op->mode, (size_t)op->index);
      if (canonicalPropertyVar.count(key) > 0) {
        varMap[op] = canonicalPropertyVar[key];
      } else {
        auto unique_name = codeGen->genUniqueName(op->name);
        canonicalPropertyVar[key] = unique_name;
        varDecls[op] = unique_name;
        insert_var(op, unique_name);
        if (util::contains(outputTensors, op->tensor)) {
          outputProperties[key] = unique_name;
        }
      }
    }
  }
};
#ifndef unimplemented
#define unimplemented                                                          \
  taco_uerror << "not implemented yet";                                        \
  return static_cast<PochiVM::Value<void>>(PochiVM::Block());
#define silence_noret                                                          \
  return static_cast<PochiVM::Value<void>>(PochiVM::Block());
#endif