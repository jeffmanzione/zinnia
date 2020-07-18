#ifndef LANG_SEMANTICS_EXPRESSIONS_POSTFIX_H_
#define LANG_SEMANTICS_EXPRESSIONS_POSTFIX_H_

#include "lang/lexer/token.h"
#include "lang/semantics/expression_macros.h"

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

#endif /* LANG_SEMANTICS_EXPRESSIONS_POSTFIX_H_ */