#ifndef TACO_BACKEND_POCHI_H
#define TACO_BACKEND_POCHI_H
#include <map>
#include <vector>

#include "taco/ir/ir.h"
#include "compile_to_pochivt.h"
#include "taco/ir/ir_printer.h"
#include "codegen.h"
#include "compile_to_pochi.h"

namespace taco {
  bool should_use_pochi_codegen(std::string fnName);
  void set_pochi_state(bool usePochi);
bool get_pochi_state();
namespace ir {


class CodeGen_Pochi : public CodeGen {
public:
  /// Initialize a code generator that generates code to an
  /// output stream
  CodeGen_Pochi(std::ostream &dest, OutputKind outputKind, std::string fnName, bool simplify=true);
  PochiVTCompiler compiler;
  ~CodeGen_Pochi();

  /// Compile a lowered function
  void compile(Stmt stmt, bool isFirst=false);

  /// For debug purposes only. Prints with correct indentation level
  void dump(std::string msg);

  /// Generate shims that unpack an array of pointers representing
  /// a mix of taco_tensor_t* and scalars into a function call
  static void generateShim(const Stmt& func, std::stringstream &stream);


protected:
  using IRPrinter::visit;

  void visit(const Function*);
  void visit(const VarDecl*);
  void visit(const Yield*);
  void visit(const Var*);
  void visit(const For*);
  void visit(const While*);
  void visit(const GetProperty*);
  void visit(const Min*);
  void visit(const Max*);
  void visit(const Allocate*);
  void visit(const Sqrt*);
  void visit(const Store*);
  void visit(const Assign*);

  std::map<Expr, std::string, ExprCompare> varMap;
  std::vector<Expr> localVars;
  std::ostream &out;
  
  OutputKind outputKind;

  std::string funcName;
  int labelCount;
  bool emittingCoroutine;

  class FindVars;

private:
  virtual std::string restrictKeyword() const { return "restrict"; }
};

} // namespace ir
} // namespace taco
#endif
