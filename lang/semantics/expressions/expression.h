// expression.h
//
// Created on: Jun 23, 2018
//     Author: Jeff Manzione

#ifndef LANG_SEMANTICS_EXPRESSIONS_EXPRESSION_H_
#define LANG_SEMANTICS_EXPRESSIONS_EXPRESSION_H_

#include <stdbool.h>
#include <stdint.h>

#include "lang/lexer/token.h"
#include "lang/parser/parser.h"
#include "lang/semantics/expressions/files.h"
#include "program/tape.h"

DefineExpression(identifier) { Token *id; };

DefineExpression(constant) {
  Token *token;
  Value value;
};

DefineExpression(string_literal) {
  Token *token;
  const char *str;
};

DefineExpression(tuple_expression) {
  Token *token;
  Expando *list;
};

DefineExpression(array_declaration) {
  Token *token;
  bool is_empty;
  ExpressionTree *exp;  // A tuple expression.
};

DefineExpression(primary_expression) {
  Token *token;
  ExpressionTree *exp;
};

typedef enum {
  Postfix_none,
  Postfix_field,
  Postfix_fncall,
  Postfix_array_index,
  Postfix_increment,
  Postfix_decrement
} PostfixType;

typedef struct {
  PostfixType type;
  union {
    const Token *id;
    ExpressionTree *exp;  // Can be NULL for empty fncall.
  };
  Token *token;
} Postfix;

int produce_postfix(int *i, int num_postfix, Expando *suffixes, Postfix **next,
                    Tape *tape);

DefineExpression(postfix_expression) {
  ExpressionTree *prefix;
  Expando *suffixes;
};

DefineExpression(range_expression) {
  Token *token;
  uint32_t num_args;
  ExpressionTree *start;
  ExpressionTree *end;
  ExpressionTree *inc;
};

typedef enum {
  Unary_unknown,
  Unary_not,
  Unary_notc,
  Unary_negate,
  Unary_const
} UnaryType;

DefineExpression(unary_expression) {
  Token *token;
  UnaryType type;
  ExpressionTree *exp;
};

typedef enum {
  BiType_unknown,

  Mult_mult,
  Mult_div,
  Mult_mod,

  Add_add,
  Add_sub,

  Rel_lt,
  Rel_gt,
  Rel_lte,
  Rel_gte,
  Rel_eq,
  Rel_neq,

  And_and,
  And_xor,
  And_or,

} BiType;

typedef struct {
  Token *token;
  BiType type;
  ExpressionTree *exp;
} BiSuffix;

#define BiDefineExpression(expr) \
  DefineExpression(expr) {       \
    ExpressionTree *exp;         \
    Expando *suffixes;           \
  }

BiDefineExpression(multiplicative_expression);
BiDefineExpression(additive_expression);
BiDefineExpression(relational_expression);
BiDefineExpression(equality_expression);
BiDefineExpression(and_expression);
BiDefineExpression(xor_expression);
BiDefineExpression(or_expression);

DefineExpression(in_expression) {
  Token *token;
  bool is_not;
  ExpressionTree *element;
  ExpressionTree *collection;
};

DefineExpression(is_expression) {
  Token *token;
  ExpressionTree *exp;
  ExpressionTree *type;
};

DefineExpression(conditional_expression) { IfElse if_else; };

DefineExpression(anon_function_definition) { Function func; };

typedef struct {
  Token *colon;
  ExpressionTree *lhs;
  ExpressionTree *rhs;
} MapDecEntry;

DefineExpression(map_declaration) {
  Token *lbrce, *rbrce;
  bool is_empty;
  Expando *entries;
};

#endif /* LANG_SEMANTICS_EXPRESSIONS_EXPRESSION_H_ */
