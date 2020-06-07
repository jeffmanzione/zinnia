/*
 * statements.h
 *
 *  Created on: Dec 29, 2019
 *      Author: Jeff
 */

#ifndef CODEGEN_EXPRESSIONS_STATEMENTS_H_
#define CODEGEN_EXPRESSIONS_STATEMENTS_H_

#include "../../datastructure/expando.h"
#include "assignment.h"
#include "expression_macros.h"

typedef struct {
  Token *if_token;
  ExpressionTree *condition;
  ExpressionTree *body;
} Conditional;

typedef struct {
  Expando *conditions;
  ExpressionTree *else_exp;
} IfElse;

void populate_if_else(IfElse *if_else, const SyntaxTree *stree);
void delete_if_else(IfElse *if_else);
int produce_if_else(IfElse *if_else, Tape *tape);

DefineExpression(compound_statement) { Expando *expressions; };

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
