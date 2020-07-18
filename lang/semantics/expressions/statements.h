/*
 * statements.h
 *
 *  Created on: Dec 29, 2019
 *      Author: Jeff
 */

#ifndef CODEGEN_EXPRESSIONS_STATEMENTS_H_
#define CODEGEN_EXPRESSIONS_STATEMENTS_H_

#include "lang/semantics/expression_macros.h"
#include "lang/semantics/expressions/assignment.h"
#include "struct/alist.h"

typedef struct {
  Token *if_token;
  ExpressionTree *condition;
  ExpressionTree *body;
} Conditional;

typedef struct {
  AList *conditions;
  ExpressionTree *else_exp;
} IfElse;

void populate_if_else(IfElse *if_else, const SyntaxTree *stree);
void delete_if_else(IfElse *if_else);
int produce_if_else(IfElse *if_else, Tape *tape);

DefineExpression(compound_statement) { AList *expressions; };

DefineExpression(try_statement) {
  ExpressionTree *try_body;
  Assignment error_assignment_lhs;
  const Token *try_token, *catch_token;
  ExpressionTree *catch_body;
};

DefineExpression(raise_statement) {
  const Token *raise_token;
  ExpressionTree *exp;
};

DefineExpression(selection_statement) { IfElse if_else; };

DefineExpression(jump_statement) {
  const Token *return_token;
  ExpressionTree *exp;
};

DefineExpression(break_statement) {
  enum { Break_break, Break_continue } type;
  const Token *token;
};

DefineExpression(exit_statement) { const Token *token; };

#endif /* CODEGEN_EXPRESSIONS_STATEMENTS_H_ */
