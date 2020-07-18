// assignment.h
//
// Created on: Dec 27, 2019
//     Author: Jeff Manzione

#ifndef LANG_SEMANTICS_EXPRESSIONS_ASSIGNMENT_H_
#define LANG_SEMANTICS_EXPRESSIONS_ASSIGNMENT_H_

#include "lang/lexer/token.h"
#include "lang/semantics/expression_macros.h"
#include "struct/alist.h"

typedef struct {
  enum { SingleAssignment_var, SingleAssignment_complex } type;
  union {
    struct {
      Token *var;
      bool is_const;
      Token *const_token;
    };
    struct {
      ExpressionTree *prefix_exp;
      AList *suffixes;  // Should be Posfixes.
    };
  };
} SingleAssignment;

typedef struct {
  AList *subargs;
} MultiAssignment;

typedef struct {
  enum { Assignment_single, Assignment_array, Assignment_tuple } type;
  union {
    SingleAssignment single;
    MultiAssignment multi;
  };
} Assignment;

DefineExpression(assignment_expression) {
  Token *eq_token;
  Assignment assignment;
  ExpressionTree *rhs;
};

Assignment populate_assignment(const SyntaxTree *stree);
void delete_assignment(Assignment *assignment);
int produce_assignment(Assignment *assign, Tape *tape, const Token *eq_token);

#endif /* LANG_SEMANTICS_EXPRESSIONS_ASSIGNMENT_H_ */
