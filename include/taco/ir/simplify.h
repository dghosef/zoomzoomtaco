#ifndef TACO_IR_SIMPLIFY_H
#define TACO_IR_SIMPLIFY_H

namespace taco {
namespace ir {
class Expr;
class Stmt;

/// Simplifies an expression (e.g. by applying algebraic identities).
ir::Expr simplify(const ir::Expr& expr);

/// Simplifies a statement (e.g. by applying constant copy propagation).
ir::Stmt simplify(const ir::Stmt& stmt);

/* REPLACING INDEXES INTO ARRAY:
  We want to replace all array indexes of the form
  for(i = start; i < end; i += increment) {
    ...
    ... arr[i] ...;
    ...
  }
  with
  arr_p = arr + start;
  for(i = start; i < end; i += increment) {
    ...
    ... *arr_p ...;
    ...
    arr_p += increment
  }
  also cache the end bound of a for loop into a variable
  */
ir::Stmt rewriteForLoops(const ir::Stmt& stmt);

}}
#endif
