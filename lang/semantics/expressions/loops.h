// loops.h
//
// Created on: Dec 28, 2019
//     Author: Jeff

#ifndef LANG_SEMANTICS_EXPRESSIONS_LOOPS_H_
#define LANG_SEMANTICS_EXPRESSIONS_LOOPS_H_

#include "lang/lexer/token.h"
#include "lang/semantics/expression_macros.h"
#include "lang/semantics/expressions/assignment.h"

DefineExpression(foreach_statement) {
  Token *for_token, *in_token;
  Assignment assignment_lhs;
  ExpressionTree *iterable;
  ExpressionTree *body;
};

DefineExpression(for_statement) {
  Token *for_token;
  ExpressionTree *init;
  ExpressionTree *condition;
  ExpressionTree *inc;
  ExpressionTree *body;
};

DefineExpression(while_statement) {
  Token *while_token;
  ExpressionTree *condition;
  ExpressionTree *body;
};

#endif /* LANG_SEMANTICS_EXPRESSIONS_LOOPS_H_ */
