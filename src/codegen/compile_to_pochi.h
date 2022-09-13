// The old version before refactoring to use pochivt
#ifndef COMPILE_TO_POCHI
#define COMPILE_TO_POCHI
#define DONT_RECOMPILE
#include <cstdint>
#include <typeinfo>
#include <variant>
#include <cstring>

#include "codegen.h"
#include "pochi_templates.h"
#include "pochivm.h"
#include "pochivm/ast_type_helper.h"
#include "runtime/pochivm_runtime_headers.h"
#include "taco/ir/ir.h"

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
extern bool debug;

class PochiCompiler : taco::ir::CodeGen {
public:
  using PochiSum = std::variant<PochiVM::Value<bool>,     // 0
                                PochiVM::Value<uint8_t>,  // 1
                                PochiVM::Value<uint16_t>, // 2
                                PochiVM::Value<uint32_t>, // 3
                                PochiVM::Value<uint64_t>, // 4
                                PochiVM::Value<int8_t>,   // 5
                                PochiVM::Value<int16_t>,  // 6
                                PochiVM::Value<int32_t>,  // 7
                                PochiVM::Value<int64_t>,  // 8
                                PochiVM::Value<float>,    // 9
                                PochiVM::Value<double>,   // 10

                                PochiVM::Value<bool *>,          // 11
                                PochiVM::Value<uint8_t *>,       // 12
                                PochiVM::Value<uint16_t *>,      // 13
                                PochiVM::Value<uint32_t *>,      // 14
                                PochiVM::Value<uint64_t *>,      // 15
                                PochiVM::Value<int8_t *>,        // 16
                                PochiVM::Value<int16_t *>,       // 17
                                PochiVM::Value<int32_t *>,       // 18
                                PochiVM::Value<int64_t *>,       // 19
                                PochiVM::Value<float *>,         // 20
                                PochiVM::Value<double *>,        // 21
                                PochiVM::Value<taco_tensor_t *>, // 22

                                PochiVM::Variable<bool>,     // 23
                                PochiVM::Variable<uint8_t>,  // 24
                                PochiVM::Variable<uint16_t>, // 25
                                PochiVM::Variable<uint32_t>, // 26
                                PochiVM::Variable<uint64_t>, // 27
                                PochiVM::Variable<int8_t>,   // 28
                                PochiVM::Variable<int16_t>,  // 29
                                PochiVM::Variable<int32_t>,  // 30
                                PochiVM::Variable<int64_t>,  // 31
                                PochiVM::Variable<float>,    // 32
                                PochiVM::Variable<double>,   // 33

                                PochiVM::Variable<bool *>,          // 34
                                PochiVM::Variable<uint8_t *>,       // 35
                                PochiVM::Variable<uint16_t *>,      // 36
                                PochiVM::Variable<uint32_t *>,      // 37
                                PochiVM::Variable<uint64_t *>,      // 38
                                PochiVM::Variable<int8_t *>,        // 39
                                PochiVM::Variable<int16_t *>,       // 40
                                PochiVM::Variable<int32_t *>,       // 41
                                PochiVM::Variable<int64_t *>,       // 42
                                PochiVM::Variable<float *>,         // 43
                                PochiVM::Variable<double *>,        // 44
                                PochiVM::Variable<taco_tensor_t *>, // 45

                                PochiVM::Reference<bool>,          // 46
                                PochiVM::Reference<uint8_t>,       // 47
                                PochiVM::Reference<uint16_t>,      // 48
                                PochiVM::Reference<uint32_t>,      // 49
                                PochiVM::Reference<uint64_t>,      // 50
                                PochiVM::Reference<int8_t>,        // 51
                                PochiVM::Reference<int16_t>,       // 52
                                PochiVM::Reference<int32_t>,       // 53
                                PochiVM::Reference<int64_t>,       // 54
                                PochiVM::Reference<float>,         // 55
                                PochiVM::Reference<double>,        // 56
                                PochiVM::Reference<taco_tensor_t>, // 57

                                PochiVM::Value<void>,     // 58
                                PochiVM::Value<void *>,   // 59
                                PochiVM::Variable<void *> // 60
                                >;

  using fnType = int (*)(taco_tensor_t **);
  std::tuple<PochiVM::Function, PochiVM::Variable<taco_tensor_t **>> fnTuple;

  PochiVM::Function fn;
  PochiVM::Variable<taco_tensor_t **> params;
  PochiCompiler(std::string fnName)
      : fnTuple(PochiVM::NewFunction<fnType>(fnName)), fn(std::get<0>(fnTuple)),
        params(std::get<1>(fnTuple)), taco::ir::CodeGen(
                                          std::cout, CodeGen::CodeGenType::C){};

  PochiSum visit_pochi(const taco::ir::Literal *op);
  PochiSum visit_pochi(const taco::ir::Var *op);
  PochiSum visit_pochi(const taco::ir::Neg *op);
  PochiSum visit_pochi(const taco::ir::Sqrt *op);
  PochiSum visit_pochi(const taco::ir::Add *op);
  PochiSum visit_pochi(const taco::ir::Sub *op);
  PochiSum visit_pochi(const taco::ir::Mul *op);
  PochiSum visit_pochi(const taco::ir::Div *op);
  PochiSum visit_pochi(const taco::ir::Rem *op);
  PochiSum visit_pochi(const taco::ir::Min *op);
  PochiSum visit_pochi(const taco::ir::Max *op);
  PochiSum visit_pochi(const taco::ir::BitAnd *op);
  PochiSum visit_pochi(const taco::ir::BitOr *op);
  PochiSum visit_pochi(const taco::ir::Eq *op);
  PochiSum visit_pochi(const taco::ir::Neq *op);
  PochiSum visit_pochi(const taco::ir::Gt *op);
  PochiSum visit_pochi(const taco::ir::Lt *op);
  PochiSum visit_pochi(const taco::ir::Gte *op);
  PochiSum visit_pochi(const taco::ir::Lte *op);
  PochiSum visit_pochi(const taco::ir::And *op);
  PochiSum visit_pochi(const taco::ir::Or *op);
  PochiSum visit_pochi(const taco::ir::Cast *op);
  PochiSum visit_pochi(const taco::ir::Call *op);
  PochiSum visit_pochi(const taco::ir::IfThenElse *op);
  PochiSum visit_pochi(const taco::ir::Case *op);
  PochiSum visit_pochi(const taco::ir::Switch *op);
  PochiSum visit_pochi(const taco::ir::Load *op);
  PochiSum visit_pochi(const taco::ir::Malloc *op);
  PochiSum visit_pochi(const taco::ir::Sizeof *op);
  PochiSum visit_pochi(const taco::ir::Store *op);
  PochiSum visit_pochi(const taco::ir::For *op);
  PochiSum visit_pochi(const taco::ir::While *op);
  PochiSum visit_pochi(const taco::ir::Block *op);
  PochiSum visit_pochi(const taco::ir::Scope *op);
  PochiSum visit_pochi(const taco::ir::Function *op);
  PochiSum visit_pochi(const taco::ir::VarDecl *op);
  PochiSum visit_pochi(const taco::ir::Assign *op);
  PochiSum visit_pochi(const taco::ir::Yield *op);
  PochiSum visit_pochi(const taco::ir::Allocate *op);
  PochiSum visit_pochi(const taco::ir::Free *op);
  PochiSum visit_pochi(const taco::ir::Comment *op);
  PochiSum visit_pochi(const taco::ir::BlankLine *op);
  PochiSum visit_pochi(const taco::ir::Print *op);
  PochiSum visit_pochi(const taco::ir::GetProperty *op);
  PochiSum visit_pochi(const taco::ir::Continue *op);
  PochiSum visit_pochi(const taco::ir::Sort *op);
  PochiSum visit_pochi(const taco::ir::Break *op);
  PochiVM::IfStatement case_visitor_helper(const taco::ir::Case *op,
                                           size_t curr_clause);
  template <typename T>
  PochiVM::IfStatement switch_visitor_helper(const taco::ir::Switch *op,
                                             size_t curr_case,
                                             T &control_expr_value) {
    auto switch_case = op->cases[curr_case];
    auto value_value_ptr = std::visit(
        [&](auto &&value) -> PochiVM::AstNodeBase * {
          using ExprType = std::decay_t<decltype(value)>;
          using InnerExprType = inner_type_t<ExprType>;
          if constexpr (std::is_arithmetic_v<InnerExprType>) {
            return value.__pochivm_value_ptr;
          } else {
            { taco_uerror << "Invalid Type for Add"; }
            __builtin_unreachable();
          }
        },
        visit_taco_op(switch_case.first));

    return std::visit(
        [&](auto &&statement) -> PochiVM::IfStatement {
          using StmtType = std::decay_t<decltype(statement)>;
          using InnerStmtType = inner_type_t<StmtType>;
          if constexpr (std::is_same_v<InnerStmtType, void>) {
            if (curr_case == op->cases.size() - 1) {
              return PochiVM::If(
                         PochiVM::Value<bool>(new PochiVM::AstComparisonExpr(
                             PochiVM::AstComparisonExprType::EQUAL,
                             control_expr_value.__pochivm_value_ptr,
                             value_value_ptr)))
                  .Then(statement);
            } else {
              return PochiVM::If(
                         PochiVM::Value<bool>(new PochiVM::AstComparisonExpr(
                             PochiVM::AstComparisonExprType::EQUAL,
                             control_expr_value.__pochivm_value_ptr,
                             value_value_ptr)))
                  .Then(statement)
                  .Else(switch_visitor_helper<T>(op, curr_case + 1,
                                                 control_expr_value));
            }
          } else {
            { taco_uerror << "Invalid Type for Add"; }
            __builtin_unreachable();
          }
        },
        visit_taco_op(switch_case.second));
  }

  // Check to make sure op is not nullptr and then visit
  template <typename T> PochiCompiler::PochiSum check_and_visit(const T op) {
#ifndef UNSAFE
    taco_iassert(op != nullptr);
#endif
    return visit_pochi(op);
  }
  class FindVars;

  std::map<std::string, PochiSum> pochiVarMap;

  PochiSum visit_taco_op(taco::ir::IRNode *op);
  PochiSum visit_taco_op(const taco::ir::IRNode *const op);
  PochiSum visit_taco_op(const taco::ir::Expr op);
  PochiSum visit_taco_op(const taco::ir::Stmt op);
  PochiVM::AstNodeBase *get_value_pointer(PochiSum);
  PochiVM::AstNodeBase *get_reference_pointer(PochiSum);
  PochiVM::Value<void> visit_taco_stmt(PochiCompiler::PochiSum stmt);
  std::string funcName;
  int labelCount;
  bool emittingCoroutine;
  std::map<taco::ir::Expr, std::string, taco::ir::ExprCompare> varMap;
  std::vector<taco::ir::Expr> localVars;
  PochiVM::Block
  emitDecls(std::map<taco::ir::Expr, std::string, taco::ir::ExprCompare> varMap,
            std::vector<taco::ir::Expr> inputs,
            std::vector<taco::ir::Expr> outputs);
  PochiVM::Value<void> unpackTensorProperty(std::string varname,
                                            const taco::ir::GetProperty *op,
                                            bool is_output_prop);
  PochiVM::Value<void> emitTensorProperty(std::string varname,
                                          const taco::ir::GetProperty *op,
                                          bool is_ptr);
  PochiVM::Value<void> emitPack(
      std::map<std::tuple<taco::ir::Expr, taco::ir::TensorProperty, int, int>,
               std::string>
          outputProperties,
      std::vector<taco::ir::Expr> outputs);
  PochiVM::Value<void> pointTensorProperty(std::string varname);
  PochiVM::Value<void> packTensorProperty(std::string varname,
                                          taco::ir::Expr tnsr,
                                          taco::ir::TensorProperty property,
                                          int mode, int index);
  void compile(taco::ir::Stmt stmt, bool isFirst = false){};
};
// find variables for generating declarations
// generates a single var for each GetProperty
class PochiCompiler::FindVars : public IRVisitor {
public:
  map<Expr, string, ExprCompare> varMap;
  map<string, PochiSum> *pochiVarMap;

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

  PochiCompiler *codeGen;

  PochiVM::Block body;
  template<typename T>
  void insert_var(std::string name) {
    PochiVM::Variable<T> pochiVar =
        codeGen->fn.NewVariable<T>();
    body.Append(PochiVM::Declare(pochiVar));
    
    pochiVarMap->insert({name, std::move(pochiVar)});
  }
  void insert_param(const Var *var) {
    // FIX TYPES DOWN HERE
    insert_var<taco_tensor_t *>(var->name);
  }

  // copy inputs and outputs into the map
  FindVars(vector<Expr> inputs, vector<Expr> outputs, PochiCompiler *codeGen,
           map<string, PochiSum> *pochiVarMap, PochiVM::Block body)
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
  insert_var<type>(name);

    switch_over_Datatype(var->type.getKind())
#undef stmt
  }

  // Insert the pointer version of the variable
  void insert_var(const GetProperty *op, string name) {
    // FIX TYPES DOWN HERE
    varMap[op] = name;
    if (op->property == TensorProperty::Values) {
#define stmt(Kind, type)                                                       \
  insert_var<type *>(name);

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
#endif