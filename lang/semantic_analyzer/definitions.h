#ifndef LANG_SEMANTIC_ANALYZER_DEFINITIONS_H_
#define LANG_SEMANTIC_ANALYZER_DEFINITIONS_H_

#include "debug/debug.h"
#include "lang/lexer/token.h"
#include "lang/semantic_analyzer/expression_tree.h"
#include "lang/semantic_analyzer/semantic_analyzer.h"
#include "program/tape.h"
#include "struct/alist.h"
#include "struct/map.h"

DEFINE_EXPRESSION_WITH_PRODUCER(identifier, Tape) { Token *id; };

DEFINE_EXPRESSION_WITH_PRODUCER(constant, Tape) {
  Token *token;
  Primitive value;
};

DEFINE_EXPRESSION_WITH_PRODUCER(string_literal, Tape) {
  Token *token;
  const char *str;
};

DEFINE_EXPRESSION_WITH_PRODUCER(tuple_expression, Tape) {
  Token *token;
  AList *list;
};

DEFINE_EXPRESSION_WITH_PRODUCER(array_declaration, Tape) {
  Token *token;
  bool is_empty;
  ExpressionTree *exp; // A tuple expression.
};

DEFINE_EXPRESSION_WITH_PRODUCER(primary_expression, Tape) {
  Token *token;
  ExpressionTree *exp;
};

DEFINE_EXPRESSION_WITH_PRODUCER(primary_expression_no_constants, Tape) {
  Token *token;
  ExpressionTree *exp;
};

typedef enum {
  Postfix_none,
  Postfix_field,
  Postfix_fncall,
  Postfix_array_index,
  // Postfix_increment,
  // Postfix_decrement
} PostfixType;

typedef struct {
  PostfixType type;
  union {
    const Token *id;
    ExpressionTree *exp; // Can be NULL for empty fncall.
  };
  Token *token;
} Postfix;

int produce_postfix(SemanticAnalyzer *analyzer, int *i, int num_postfix,
                    AList *suffixes, Postfix **next, Tape *tape);

DEFINE_EXPRESSION_WITH_PRODUCER(postfix_expression, Tape) {
  ExpressionTree *prefix;
  AList *suffixes;
};

DEFINE_EXPRESSION_WITH_PRODUCER(range_expression, Tape) {
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
  Unary_const,
  Unary_await
} UnaryType;

DEFINE_EXPRESSION_WITH_PRODUCER(unary_expression, Tape) {
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
  And_or,

  Bin_and,
  Bin_xor,
  Bin_or

} BiType;

typedef struct {
  Token *token;
  BiType type;
  ExpressionTree *exp;
} BiSuffix;

#define DEFINE_BI_EXPRESSION(expr)                                             \
  DEFINE_EXPRESSION_WITH_PRODUCER(expr, Tape) {                                \
    ExpressionTree *exp;                                                       \
    AList *suffixes;                                                           \
  }

DEFINE_BI_EXPRESSION(multiplicative_expression);
DEFINE_BI_EXPRESSION(additive_expression);
DEFINE_BI_EXPRESSION(relational_expression);
DEFINE_BI_EXPRESSION(equality_expression);
DEFINE_BI_EXPRESSION(and_expression);
DEFINE_BI_EXPRESSION(or_expression);
DEFINE_BI_EXPRESSION(binary_and_expression);
DEFINE_BI_EXPRESSION(binary_xor_expression);
DEFINE_BI_EXPRESSION(binary_or_expression);

DEFINE_EXPRESSION_WITH_PRODUCER(in_expression, Tape) {
  Token *token;
  bool is_not;
  ExpressionTree *element;
  ExpressionTree *collection;
};

DEFINE_EXPRESSION_WITH_PRODUCER(is_expression, Tape) {
  Token *token;
  ExpressionTree *exp;
  ExpressionTree *type;
};

typedef struct {
  Token *if_token;
  ExpressionTree *condition;
  ExpressionTree *body;
} Conditional;

typedef struct {
  AList *conditions;
  ExpressionTree *else_exp;
} IfElse;

DEFINE_EXPRESSION_WITH_PRODUCER(conditional_expression, Tape) {
  IfElse if_else;
};

typedef struct {
  bool is_const, is_field, has_default;
  const Token *arg_name;
  const Token *const_token;
  const Token *field_token;
  ExpressionTree *default_value;
} Argument;

typedef struct {
  const Token *token;
  int count_required, count_optional;
  AList *args;
} Arguments;

typedef enum {
  SpecialMethod__NONE,
  SpecialMethod__EQUIV,
  SpecialMethod__NEQUIV,
  SpecialMethod__ARRAY_INDEX,
  SpecialMethod__ARRAY_SET
} SpecialMethod;

typedef struct {
  const Token *def_token;
  const Token *fn_name;
  SpecialMethod special_method;
  const Token *const_token, *async_token;
  bool has_args, is_const, is_async;
  Arguments args;
  ExpressionTree *body;
} FunctionDef;

DEFINE_EXPRESSION_WITH_PRODUCER(anon_function_definition, Tape) {
  FunctionDef func;
};

typedef struct {
  Token *colon;
  ExpressionTree *lhs;
  ExpressionTree *rhs;
} MapDecEntry;

DEFINE_EXPRESSION_WITH_PRODUCER(map_declaration, Tape) {
  Token *lbrce, *rbrce;
  bool is_empty;
  AList *entries;
};

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
      AList *suffixes; // Should be Posfixes.
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

DEFINE_EXPRESSION_WITH_PRODUCER(assignment_expression, Tape) {
  Token *eq_token;
  Assignment assignment;
  ExpressionTree *rhs;
};

  // DEFINE_EXPRESSION_WITH_PRODUCER(compound_statement, Tape) {
  //   AList *expressions;
  // };

  // DEFINE_EXPRESSION_WITH_PRODUCER(try_statement, Tape) {
  //   ExpressionTree *try_body;
  //   Assignment error_assignment_lhs;
  //   const Token *try_token, *catch_token;
  //   ExpressionTree *catch_body;
  // };

  // DEFINE_EXPRESSION_WITH_PRODUCER(raise_statement, Tape) {
  //   const Token *raise_token;
  //   ExpressionTree *exp;
  // };

  // DEFINE_EXPRESSION_WITH_PRODUCER(selection_statement, Tape) { IfElse
  // if_else; };

  // DEFINE_EXPRESSION_WITH_PRODUCER(jump_statement, Tape) {
  //   const Token *return_token;
  //   ExpressionTree *exp;
  // };

  // DEFINE_EXPRESSION_WITH_PRODUCER(break_statement, Tape) {
  //   enum { Break_break, Break_continue } type;
  //   const Token *token;
  // };

  // DEFINE_EXPRESSION_WITH_PRODUCER(exit_statement, Tape) { const Token *token;
  // };

#endif /* LANG_SEMANTIC_ANALYZER_DEFINITIONS_H_ */