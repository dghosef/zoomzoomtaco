#include "taco/ir/simplify.h"

#include <map>
#include <queue>

#include "taco/ir/ir.h"
#include "taco/ir/ir_visitor.h"
#include "taco/ir/ir_rewriter.h"
#include "taco/util/strings.h"
#include "taco/util/collections.h"
#include "taco/util/scopedmap.h"

namespace taco {
namespace ir {

struct ExpressionSimplifier : public IRRewriter {
  using IRRewriter::visit;

  void visit(const Cast* op) {
    Expr a = rewrite(op->a);

    // (T)true == T(1)
    // (T)false == T(0)
    if (isa<Literal>(a) && to<Literal>(a)->type.isBool()) {
      const auto lit = TypedComponentVal(op->type, 
                                         to<Literal>(a)->getValue<bool>());
      expr = Literal::make(lit, op->type);
      return;
    }

    if (a == op->a) {
      expr = op;
    }
    else {
      expr = Cast::make(a, op->type);
    }
  }

  void visit(const Or* op) {
    Expr a = rewrite(op->a);
    Expr b = rewrite(op->b);

    // true || b = true
    // false || b = b
    if (isa<Literal>(a)) {
      auto literal = to<Literal>(a);
      expr = literal->getValue<bool>() ? a : b;
      return;
    }

    // a || true = true
    // a || false = a
    if (isa<Literal>(b)) {
      auto literal = to<Literal>(b);
      expr = literal->getValue<bool>() ? b : a;
      return;
    }

    if (a == op->a && b == op->b) {
      expr = op;
    }
    else {
      expr = Or::make(a, b);
    }
  }

  void visit(const And* op) {
    Expr a = rewrite(op->a);
    Expr b = rewrite(op->b);

    // true && b = b
    // false && b = false
    if (isa<Literal>(a)) {
      auto literal = to<Literal>(a);
      expr = literal->getValue<bool>() ? b : a;
      return;
    }

    // a && true = a
    // a && false = false
    if (isa<Literal>(b)) {
      auto literal = to<Literal>(b);
      expr = literal->getValue<bool>() ? a : b;
      return;
    }

    if (a == op->a && b == op->b) {
      expr = op;
    }
    else {
      expr = And::make(a, b);
    }
  }

  void visit(const Add* op) {
    Expr a = rewrite(op->a);
    Expr b = rewrite(op->b);

    // 1 + 1 = 2
    if (isa<Literal>(a) && isa<Literal>(b)) {
      auto lita = to<Literal>(a);
      auto litb = to<Literal>(b);
      auto typea = lita->type;
      auto typeb = litb->type;
      auto resulttype = max_type(typea, typeb);
      if(isScalar(typea) && isScalar(typeb) && (resulttype.isInt() || resulttype.isUInt())) {
        // convert literals to result type uint -> int
        auto litaval = lita->getTypedVal();
        auto litbval = litb->getTypedVal();
        if (typea != resulttype) {
          litaval = TypedComponentVal(resulttype, (int) litaval.getAsIndex());
        }
        if (typeb != resulttype) {
          litbval = TypedComponentVal(resulttype, (int) litbval.getAsIndex());
        }
        expr = Literal::make(litaval + litbval, resulttype);
        return; 
      }
    }

    // (c + d) + b = c + (d + b)
    // TODO: handle operands of different types
    if (isa<Add>(a) && isa<Literal>(to<Add>(a)->b) && isa<Literal>(b)) {
      auto adda = to<Add>(a);
      auto litd = to<Literal>(adda->b);
      auto litb = to<Literal>(b);
      auto typec = adda->a.type();
      auto typed = litd->type;
      auto typeb = litb->type;
      if (typec == typed && typed == typeb && isScalar(typeb) && (typeb.isInt() || typeb.isUInt())){
        auto litdval = litd->getTypedVal();
        auto litbval = litb->getTypedVal();
        expr = simplify(Add::make(adda->a, Literal::make(litdval + litbval, typeb)));
        return;
      }
    }

    // (c - d) + b = c + (b - d)
    // TODO: handle operands of different types
    if (isa<Sub>(a) && isa<Literal>(to<Sub>(a)->b) && isa<Literal>(b)) {
      auto suba = to<Sub>(a);
      auto litd = to<Literal>(suba->b);
      auto litb = to<Literal>(b);
      auto typec = suba->a.type();
      auto typed = litd->type;
      auto typeb = litb->type;
      if (typec == typed && typed == typeb && isScalar(typeb) && (typeb.isInt() || typeb.isUInt())){
        auto litdval = litd->getTypedVal();
        auto litbval = litb->getTypedVal();
        expr = simplify(Add::make(suba->a, Literal::make(litbval - litdval, typeb)));
        return;
      }
    }

    // 0 + b = b
    if (isa<Literal>(a)) {
      auto literal = to<Literal>(a);
      if (literal->equalsScalar(0)) {
        expr = b;
        return;
      }
    }

    // a + 0 = a
    if (isa<Literal>(b)) {
      auto literal = to<Literal>(b);
      if (literal->equalsScalar(0)) {
        expr = a;
        return;
      }
    }

    if (a == op->a && b == op->b) {
      expr = op;
    }
    else {
      expr = Add::make(a, b);
    }
  }

  void visit(const Sub* op) {
    Expr a = rewrite(op->a);
    Expr b = rewrite(op->b);

    // 2 - 1 = 1
    if (isa<Literal>(a) && isa<Literal>(b)) {
      auto lita = to<Literal>(a);
      auto litb = to<Literal>(b);
      auto typea = lita->type;
      auto typeb = litb->type;
      auto resulttype = max_type(typea, typeb);
      if(isScalar(typea) && isScalar(typeb) && (resulttype.isInt() || resulttype.isUInt())) {
        // convert literals to result type uint -> int
        auto litaval = lita->getTypedVal();
        auto litbval = litb->getTypedVal();
        if (typea != resulttype) {
          litaval = TypedComponentVal(resulttype, (int) litaval.getAsIndex());
        }
        if (typeb != resulttype) {
          litbval = TypedComponentVal(resulttype, (int) litbval.getAsIndex());
        }
        expr = Literal::make(litaval - litbval, resulttype);
        return; 
      }
    }

    // 0 - b = -b
    if (isa<Literal>(a)) {
      auto literal = to<Literal>(a);
      if (literal->equalsScalar(0)) {
        expr = Neg::make(b);
        return;
      }
    }

    // a - 0 = a
    if (isa<Literal>(b)) {
      auto literal = to<Literal>(b);
      if (literal->equalsScalar(0)) {
        expr = a;
        return;
      }
    }

    if (a == op->a && b == op->b) {
      expr = op;
    }
    else {
      expr = Sub::make(a, b);
    }
  }

  void visit(const Mul* op) {
    Expr a = rewrite(op->a);
    Expr b = rewrite(op->b);

    // a * b = ab
    if (isa<Literal>(a) && isa<Literal>(b)) {
      auto lita = to<Literal>(a);
      auto litb = to<Literal>(b);
      auto typea = lita->type;
      auto typeb = litb->type;
      auto resulttype = max_type(typea, typeb);
      if(isScalar(typea) && isScalar(typeb) && (resulttype.isInt() || resulttype.isUInt())) {
        // convert literals to result type uint -> int
        auto litaval = lita->getTypedVal();
        auto litbval = litb->getTypedVal();
        if (typea != resulttype) {
          litaval = TypedComponentVal(resulttype, (int) litaval.getAsIndex());
        }
        if (typeb != resulttype) {
          litbval = TypedComponentVal(resulttype, (int) litbval.getAsIndex());
        }
        expr = Literal::make(litaval * litbval, resulttype);
        return; 
      }
    }

    // (c * d) * b = c * db
    if (isa<Mul>(a) && isa<Literal>(to<Mul>(a)->b) && isa<Literal>(b)) {
      auto mula = to<Mul>(a);
      auto litd = to<Literal>(mula->b);
      auto litb = to<Literal>(b);
      auto typec = mula->a.type();
      auto typed = litd->type;
      auto typeb = litb->type;
      if (typec == typed && typed == typeb && isScalar(typeb) && (typeb.isInt() || typeb.isUInt())){
        auto litdval = litd->getTypedVal();
        auto litbval = litb->getTypedVal();
        expr = simplify(Mul::make(mula->a, Literal::make(litdval * litbval, typeb)));
        return;
      }
    }

    // 0 * b = 0
    // 1 * b = b
    if (isa<Literal>(a)) {
      auto literal = to<Literal>(a);
      if (literal->equalsScalar(0)) {
        expr = literal;
        return;
      }
      else if(literal->equalsScalar(1)) {
        expr = b;
        return;
      }
    }

    // a * 0 = 0
    // a * 1 = a
    if (isa<Literal>(b)) {
      auto literal = to<Literal>(b);

      if (literal->equalsScalar(0)) {
        expr = literal;
        return;
      }
      else if(literal->equalsScalar(1)) {
        expr = a;
        return;
      }
    }

    if (a == op->a && b == op->b) {
      expr = op;
    }
    else {
      expr = Mul::make(a, b);
    }
  }

  void visit(const Div* op) {
    Expr a = rewrite(op->a);
    Expr b = rewrite(op->b);

    if (isa<Literal>(a) && isa<Literal>(b)) {
      auto lita = to<Literal>(a);
      auto litb = to<Literal>(b);
      auto typea = lita->type;
      auto typeb = litb->type;
      if (typea == typeb && isScalar(typea)) {
        if (typea.isInt()) {
          expr = Literal::make(lita->getIntValue()/litb->getIntValue(), typea);
          return;
        }
        else if (typea.isUInt()) {
          expr = Literal::make(lita->getUIntValue()/litb->getUIntValue(), typea);
          return;
        }
      }
    }

    // a / 1 = a
    if (isa<Literal>(b)) {
      auto literal = to<Literal>(b);

      if (literal->equalsScalar(1)) {
        expr = a;
        return;
      }
    }

    if (a == op->a && b == op->b) {
      expr = op;
    }
    else {
      expr = Div::make(a, b);
    }
  }
};

ir::Expr simplify(const ir::Expr& expr) {
  return ExpressionSimplifier().rewrite(expr);
}

struct rewriteForBound : IRRewriter {
  void visit(const For* op) {
    Expr var       = rewrite(op->var);
    Expr start     = rewrite(op->start);
    Expr end       = rewrite(op->end);
    Expr increment = rewrite(op->increment);
    Stmt contents  = rewrite(op->contents);
    // TODO: Replace these
    if (var == op->var && start == op->start && end == op->end &&
        increment == op->increment && contents == op->contents) {
      stmt = op;
    }
    else {
      stmt = For::make(var, start, end, increment, contents, op->kind,
                      op->parallel_unit, op->unrollFactor, op->vec_width);
    }

    static int num_vars_cached = 0;
    taco::ir::Block *ret = new taco::ir::Block;
    if(end.ptr->type_info() != IRNodeType::Var && end.ptr->type_info() != IRNodeType::GetProperty) {
      std::stringstream varname;
      varname << "endbound_" << num_vars_cached;
      num_vars_cached++;
      auto var = Var::make(varname.str(), end.type());
      auto vardecl = VarDecl::make(var, end);
      ret->append(vardecl);
      end = var;
    }      
    ret->append(For::make(op->var, op->start, end, op->increment, op->contents, op->kind, op->parallel_unit, 
                      op->unrollFactor, op->vec_width));
    stmt = ret;
    }
};
struct rewriteArrayAccesses : IRRewriter {
  std::map<std::string /*index var name*/, 
            std::set<std::string> /*relevant array accesses*/> accesses;
  std::map<std::string, Expr> varmap;
  std::map<std::string, Expr> origmap; // map from ptr variables to original variables
  // TODO: Add check to see that there are no labels(also for bounds rewrite)
  // TODO: Add check to make sure these are legal rewrites
  void visit(const Load* op) {
    Expr arr = rewrite(op->arr);
    Expr loc = rewrite(op->loc);
    std::string loc_name;
    std::string arr_name;
    Datatype arr_type;
    // todo: maybe also figure out w/ getproperty
    if(isa<Var>(loc)) {
      loc_name = to<Var>(loc)->name;
    }
    if(isa<GetProperty>(arr)) {
      arr_name = to<GetProperty>(arr)->name;
      arr_type = to<GetProperty>(arr)->type;
    }
    // check do see if loc is an index variable access
    if(loc_name != "" && arr_name != "" && accesses.count(loc_name) > 0) {
      std::string arr_ptr_name = arr_name + "_p";
      accesses[loc_name].insert(arr_ptr_name);
      if(varmap.count(arr_ptr_name) == 0) {
        varmap.insert({arr_ptr_name, Var::make(arr_ptr_name, arr_type, true /*isPtr*/)});
        origmap.insert({arr_ptr_name, arr});
      }
      expr = Load::make(varmap[arr_ptr_name], 0);
    }
    else if (arr == op->arr && loc == op->loc) {
      expr = op;
    }
    else {
      expr = loc.defined() ? Load::make(arr, loc) : Load::make(arr);
    }
  }
  void visit(const Assign* op) {
    Expr lhs = rewrite(op->lhs);
    Expr rhs = rewrite(op->rhs);
    if (lhs == op->lhs && rhs == op->rhs) {
      stmt = op;
    }
    else {
      stmt = Assign::make(lhs, rhs, op->use_atomics);
    }
  }
  void visit(const For* op) {
    // TODO: make sure increment is a literal, do start bounds/end bounds, 
    // also make sure start is a literal/variable
    Expr var       = rewrite(op->var);
    Expr start     = rewrite(op->start);
    Expr end       = rewrite(op->end);
    Expr increment = rewrite(op->increment);
    std::string varname = "";
    if(isa<Var>(var)) {
      varname = to<Var>(var)->name;
      accesses.insert({varname, {}});
    }
    Stmt contents  = rewrite(op->contents);

    Block* increment_ptrs = new Block;
    Block* decls = new Block;
    for(auto var_name : accesses[varname]) {
      auto var = varmap[var_name]; //TODO:rename var
      auto orig = origmap[var_name];
      increment_ptrs->append(Assign::make(var, Add::make(var, increment)));
      // add declarations of newly created _p variables
      decls->append(VarDecl::make(var, Add::make(orig, start)));
    }
    if(varname != "i111pwtk" || true)
        contents = Block::make(contents, increment_ptrs);
    else 
        contents = Block::make(contents, Block::make());
    if (var == op->var && start == op->start && end == op->end &&
        increment == op->increment && contents == op->contents) {
      stmt = op;
    }
    else {
      stmt = For::make(var, start, end, increment, contents, op->kind,
                      op->parallel_unit, op->unrollFactor, op->vec_width);
    }
    stmt = Block::make(decls, stmt);
  }
};
// rewrite, for example
// int i = x + b * c
// arr[i]
// with
// arr[x + b * c]
struct getIdxVarsCounts : IRVisitor {
  std::map<std::string, Expr> varInits;
  std::map<std::string, int> varCounts;
  void visit(const VarDecl* op) {
    if(op->var.as<Var>()) {
      auto v= to<Var>(op->var);
      std::string name = v->name;
      varInits.insert({name, op->rhs});
    }
  }
  void visit(const Var* op) {
    varCounts[op->name]++;
  }
};
struct replaceIdxVarsOnlyUsedOnce : IRRewriter {
  std::set<std::string> removedVars;
  std::map<std::string, Expr> varInits;
  std::map<std::string, int> varCounts;
  void visit(const Var* op) {
    if(varCounts[op->name] == 1 && inArrIdx) {
      expr = varInits[op->name];
      removedVars.insert(op->name);
    } else {
      expr = op;
    }
  }
  bool inArrIdx = false;
  void visit(const Store* op) {
    Expr arr  = rewrite(op->arr);
    inArrIdx = true;
    Expr loc  = rewrite(op->loc);
    inArrIdx = false;
    Expr data = rewrite(op->data);
    if (arr == op->arr && loc == op->loc && data == op->data) {
      stmt = op;
    }
    else {
      stmt = Store::make(arr, loc, data, op->use_atomics);
    }
  }
  void visit(const Load* op) {
    Expr arr = rewrite(op->arr);
    inArrIdx = true;
    Expr loc = rewrite(op->loc);
    inArrIdx = false;
    if (arr == op->arr && loc == op->loc) {
      expr = op;
    }
    else {
      expr = loc.defined() ? Load::make(arr, loc) : Load::make(arr);
    }
  }
};

struct rewriteTacoMin : IRRewriter {
  void visit(const VarDecl *op) {
    Expr var = rewrite(op->var);
    Expr rhs = rewrite(op->rhs);
    if(isa<Min>(rhs) && to<Min>(rhs)->operands.size() == 2) {
      auto a = to<Min>(rhs)->operands[0];
      auto b = to<Min>(rhs)->operands[1];
      auto assignstmt = IfThenElse::make(Lt::make(a, b), Assign::make(var, a), Assign::make(var, b));
      auto decl = VarDecl::make(var, Literal::make(0, var.type()));
      stmt = Block::make(decl, assignstmt);
    }
    else if(isa<Min>(rhs) && to<Min>(rhs)->operands.size() == 3) {
      auto a = to<Min>(rhs)->operands[0];
      auto b = to<Min>(rhs)->operands[1];
      auto c = to<Min>(rhs)->operands[2];
      auto assignstmt = IfThenElse::make(
        And::make(Lt::make(a, b), Lt::make(a, c)), // if a < b and a < c
        Assign::make(var, a),                     //    then var = a
        IfThenElse::make(                         // else if b < c
          Lt::make(b, c),
          Assign::make(var, b),                     // then var = b
          Assign::make(var, c)                      // else var = c
        )
      );
      auto decl = VarDecl::make(var, Literal::make(0, var.type()));
      stmt = Block::make(decl, assignstmt);
      // stmt = op;
    }
    else if (var == op->var && rhs == op->rhs) {
      stmt = op;
    }
    else {
      stmt = VarDecl::make(var, rhs);
    }
  }
};

ir::Stmt rewriteForLoops(const ir::Stmt& stmt) {
  return stmt;
  ir::Stmt rewritten = rewriteForBound().rewrite(stmt);
  rewritten = rewriteTacoMin().rewrite(stmt);
  return rewritten;
  // TODO: add these back in
  getIdxVarsCounts counts;
  stmt.accept(&counts);
  replaceIdxVarsOnlyUsedOnce replacer;
  replacer.varCounts = counts.varCounts;
  replacer.varInits = counts.varInits;
  rewritten = replacer.rewrite(rewritten);
  // rewritten = rewriteArrayAccesses().rewrite(rewritten);
  return rewritten;
}

ir::Stmt simplify(const ir::Stmt& stmt) {
  // Perform copy propagation on variables that are added to a product of zero
  // and never re-assign, e.g. `int B1_pos = (0 * 42) + iB;`. These occur when
  // emitting code for top levels that are dense.

  // Identify all loop dependent variables. (This analysis is imprecise.)
  struct FindLoopDependentVars : public IRVisitor {
    std::set<Expr> loopDependentVars;
    std::map<Expr,int> defLevel;
    int loopLevel = 0;

    using IRVisitor::visit;

    void visit(const For* op) {
      loopDependentVars.insert(op->var);
      loopLevel++;
      op->contents.accept(this);
      loopLevel--;
    }

    void visit(const While* op) {
      loopLevel++;
      op->contents.accept(this);
      loopLevel--;
    }

    void visit(const VarDecl* op) {
      defLevel.insert({op->var, loopLevel});
    }

    void visit(const Assign* op) {
      if (util::contains(defLevel, op->lhs) && 
          defLevel.at(op->lhs) < loopLevel) {
        loopDependentVars.insert(op->lhs);
      }
    }
  };
  FindLoopDependentVars findLoopDepVars;
  stmt.accept(&findLoopDepVars);

  // Copy propagation (remove candidate var definitions and replace uses) and
  // expression simplification. Also identify non-redundant variable 
  // declarations.
  // TODO: Currently does not handle the following pattern:
  //   b = a
  //   if (...) {
  //     ... = b
  //     b = d
  //   } else {
  //     ... = b
  //   }
  //   ... = b
  // The use of `b` in the if branch would be rewritten to `a` but not the use
  // in the else branch. To fix this, the visit method for if and case
  // statements would probably need to be overridden so that each branch is
  // evaluated starting with the same `varsToReplace`.
  struct Simplifier : public ExpressionSimplifier {
    util::ScopedMap<Expr,std::pair<Expr,Stmt>> varsToReplace;
    std::set<Stmt> necessaryDecls;
    std::multimap<Expr,Expr> dependencies;
    util::ScopedMap<Expr,Stmt> declarations;
    std::set<Expr> loopDependentVars;

    using ExpressionSimplifier::visit;

    Simplifier(const std::set<Expr>& loopDependentVars) : 
        loopDependentVars(loopDependentVars) {}

    void visit(const Scope* scope) {
      declarations.scope();
      varsToReplace.scope();
      IRRewriter::visit(scope);
      varsToReplace.unscope();
      declarations.unscope();
    }

    void visit(const VarDecl* decl) {
      Expr rhs = rewrite(decl->rhs);
      stmt = (rhs == decl->rhs) ? decl : VarDecl::make(decl->var, rhs);

      declarations.insert({decl->var, stmt});
      if (decl->var.type().isInt() && isa<Var>(rhs) && 
          !util::contains(loopDependentVars, decl->var)) {
//        TODO: taco_iassert(!varsToReplace.contains(decl->var))
//            << "Copy propagation pass currently does not support variables "
//            << "with same name declared in nested scopes: " << decl->var.as<Var>()->name;
        varsToReplace.insert({decl->var, {rhs, stmt}});
        dependencies.insert({rhs, decl->var});
      }
    }

    void visit(const Assign* assign) {
      Expr lhs = isa<Var>(assign->lhs) ? assign->lhs : rewrite(assign->lhs);
      Expr rhs = rewrite(assign->rhs);
      stmt = (lhs == assign->lhs && rhs == assign->rhs) ? assign : 
             Assign::make(lhs, rhs, assign->use_atomics);
      
      if (declarations.contains(lhs)) {
        taco_iassert(isa<Var>(lhs));
        necessaryDecls.insert(declarations.get(lhs));
      }

      if (!assign->lhs.type().isInt()) {
        return;
      }
      
      std::queue<Expr> invalidVars;
      invalidVars.push(assign->lhs);

      while (!invalidVars.empty()) {
        Expr invalidVar = invalidVars.front();
        invalidVars.pop();

        if (varsToReplace.contains(invalidVar)){
          varsToReplace.remove(invalidVar);
        }

        auto range = dependencies.equal_range(invalidVar);
        for (auto dep = range.first; dep != range.second; ++dep) {
          invalidVars.push(dep->second);
        }
      }
    }

    void visit(const Var* var) {
      expr = varsToReplace.contains(var) ? varsToReplace.get(var).first : var;
      if (declarations.contains(expr)) {
        necessaryDecls.insert(declarations.get(expr));
      }
    }
  };
  Simplifier copyPropagation(findLoopDepVars.loopDependentVars);
  Stmt simplifiedStmt = copyPropagation.rewrite(stmt);

  // Remove redundant variable declarations.
  struct RemoveRedundantStmts : public IRRewriter {
    std::set<Stmt> necessaryDecls;
    
    using IRRewriter::visit;

    RemoveRedundantStmts(const std::set<Stmt>& necessaryDecls) : 
        necessaryDecls(necessaryDecls) {}

    void visit(const VarDecl* decl) {
      if (isa<ir::Call>(decl->rhs)) { // don't remove function calls that might have side effects
        stmt = decl;
        return;
      }
      stmt = util::contains(necessaryDecls, decl)? decl : Block::make();
    }
  };
  simplifiedStmt = RemoveRedundantStmts(copyPropagation.necessaryDecls).rewrite(
      simplifiedStmt);

  // Eliminate loops that only increment variables.
  struct RemoveRedundantLoops : public IRRewriter {
    using IRRewriter::visit;

    struct DuplicateBody : public IRRewriter {
      Expr loopCount;

      using IRRewriter::visit;

      DuplicateBody(Expr loopCount) : loopCount(loopCount) {}

      void visit(const Block* op) {
        std::vector<Stmt> contents;
        for (auto& content : op->contents) {
          Stmt rewrittenContent = rewrite(content);

          if (!rewrittenContent.defined()) {
            stmt = Stmt();
            return;
          }
          
          contents.push_back(rewrittenContent);
        }
        stmt = Block::make(contents);
      }

      void visit(const Scope* op) {
        Stmt scopedStmt = rewrite(op->scopedStmt);
        if (!scopedStmt.defined()) {
          stmt = Stmt();
          return;
        }
        stmt = Scope::make(scopedStmt);
      }

      void visit(const Assign* op) {
        if (isa<Literal>(op->rhs)) {
          stmt = op;
          return;
        }

        if (!isa<Add>(op->rhs)) {
          stmt = Stmt();
          return;
        }

        const auto add = to<Add>(op->rhs);
        if (add->a != op->lhs || !isa<Literal>(add->b)) {
          stmt = Stmt();
          return;
        }

        Expr inc = simplify(Mul::make(loopCount, add->b));
        stmt = Assign::make(op->lhs, Add::make(add->a, inc));
      }

      void visit(const IfThenElse* op) {
        stmt = Stmt();
      }
      void visit(const Case* op) {
        stmt = Stmt();
      }
      void visit(const Switch* op) {
        stmt = Stmt();
      }
      void visit(const Malloc* op) {
        stmt = Stmt();
      }
      void visit(const Store* op) {
        stmt = Stmt();
      }
      void visit(const For* op) {
        stmt = Stmt();
      }
      void visit(const While* op) {
        stmt = Stmt();
      }
      void visit(const Function* op) {
        stmt = Stmt();
      }
      void visit(const VarDecl* op) {
        stmt = Stmt();
      }
      void visit(const Yield* op) {
        stmt = Stmt();
      }
      void visit(const Allocate* op) {
        stmt = Stmt();
      }
      void visit(const Free* op) {
        stmt = Stmt();
      }
      void visit(const Comment* op) {
        stmt = Stmt();
      }
      void visit(const BlankLine* op) {
        stmt = Stmt();
      }
      void visit(const Continue* op) {
        stmt = Stmt();
      }
      void visit(const Print* op) {
        stmt = Stmt();
      }
      void visit(const GetProperty* op) {
        stmt = Stmt();
      }
      void visit(const Sort *op) {
        stmt = Stmt();
      }
      void visit(const Break *op) {
        stmt = Stmt();
      }
    };

    struct CheckModified : public IRVisitor {
      bool isModified = false;
      Expr var;
      
      using IRVisitor::visit;

      CheckModified(Expr var) : var(var) {}

      void visit(const Assign* op) {
        if (isa<Var>(op->lhs) && to<Var>(op->lhs) == var) {
          isModified = true;
        }
      }
    };

    // Replace loops of the form:
    //
    //   for (int i = s; i < e; i += c) {
    //     x += lit;
    //     y = lit;
    //   }
    //
    // with the following:
    //
    //   if (s < e)
    //     x += lit * (e - s) / c;
    //     y = lit;
    void visit(const For* op) {
      Expr loopCount = simplify(Div::make(Sub::make(op->end, op->start), 
                                          op->increment));
      Stmt body = DuplicateBody(loopCount).rewrite(op->contents);
      if (body.defined()) {
        stmt = IfThenElse::make(Lt::make(op->start, op->end), body);
        return;
      }
      IRRewriter::visit(op);
    }

    // Replace loops of the form:
    //
    //   while (i < e) {
    //     x += lit;
    //     y = lit;
    //     i++;
    //   }
    //
    // with the following:
    //
    //   if (i < e) {
    //     x += lit * (e - i);
    //     y = lit;
    //     i = e;
    //   }
    void visit(const While* op) {
      if (isa<Lt>(op->cond)) {
        const auto cond = to<Lt>(op->cond);
        if (isa<Var>(cond->a) && isa<Var>(cond->b)) {
          Expr iter = to<Var>(cond->a);
          Expr end = to<Var>(cond->b);
          CheckModified checkModified(end);
          op->contents.accept(&checkModified);
          if (!checkModified.isModified) {
            Stmt body = op->contents.as<Scope>()->scopedStmt;
            if (isa<Block>(body)) {
              auto contents = to<Block>(body)->contents;
              Stmt last = contents.back();
              if (isa<Assign>(last)) {
                const auto assign = to<Assign>(last);
                if (isa<Add>(assign->rhs) && assign->lhs == iter) {
                  const auto add = to<Add>(assign->rhs);
                  if (add->a == assign->lhs && isa<Literal>(add->b)) {
                    const auto inc = to<Literal>(add->b);
                    if ((inc->type.isInt()  && inc->equalsScalar(1)) ||
                        (inc->type.isUInt() && inc->equalsScalar(1))) {
                      Expr loopCount = simplify(Sub::make(end, iter));
                      contents.pop_back();
                      body = Block::make(contents);
                      body = DuplicateBody(loopCount).rewrite(body);
                      if (body.defined()) {
                        body = Block::make(body, Assign::make(iter, end));
                        stmt = IfThenElse::make(op->cond, body);
                        return;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
      IRRewriter::visit(op);
    }
  };
  simplifiedStmt = RemoveRedundantLoops().rewrite(simplifiedStmt);

  return simplifiedStmt;
}

}}
