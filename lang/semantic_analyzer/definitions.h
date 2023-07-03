#ifndef LANG_SEMANTIC_ANALYZER_DEFINITIONS_H_
#define LANG_SEMANTIC_ANALYZER_DEFINITIONS_H_

#include "debug/debug.h"
#include "lang/lexer/token.h"
#include "lang/semantic_analyzer/expression_tree.h"
#include "lang/semantic_analyzer/semantic_analyzer.h"
#include "program/tape.h"
#include "struct/alist.h"
#include "struct/map.h"

DEFINE_SEMANTIC_ANALYZER_PRODUCE_FN(Tape);

void semantic_analyzer_init_fn(Map *populators, Map *producers, Map *deleters);

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

typedef struct {
  const Token *id;
  const Token *colon;
  ExpressionTree *value;
} NamedArgument;

DEFINE_EXPRESSION_WITH_PRODUCER(named_argument_list, Tape) {
  Token *token;
  // NamedArgument
  AList *list;
};

DEFINE_EXPRESSION_WITH_PRODUCER(named_argument, Tape) { NamedArgument arg; };

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
  bool is_named;
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
  const Token *prefix;
  const Token *class_name;
  bool is_called, has_args;
  ExpressionTree *args_tuple;
} Annotation;

typedef struct {
  const Token *def_token;
  const Token *fn_name;
  SpecialMethod special_method;
  AList annots;
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

Assignment populate_assignment(SemanticAnalyzer *analyzer,
                               const SyntaxTree *stree);
void delete_assignment(SemanticAnalyzer *analyzer, Assignment *assignment);
int produce_assignment(SemanticAnalyzer *analyzer, Assignment *assign,
                       Tape *tape, const Token *eq_token);

DEFINE_EXPRESSION_WITH_PRODUCER(assignment_expression, Tape) {
  Token *eq_token;
  Assignment assignment;
  ExpressionTree *rhs;
};

DEFINE_EXPRESSION_WITH_PRODUCER(compound_statement, Tape) {
  AList *expressions;
};

DEFINE_EXPRESSION_WITH_PRODUCER(try_statement, Tape) {
  ExpressionTree *try_body;
  Assignment error_assignment_lhs;
  const Token *try_token, *catch_token;
  ExpressionTree *catch_body;
};

DEFINE_EXPRESSION_WITH_PRODUCER(raise_statement, Tape) {
  const Token *raise_token;
  ExpressionTree *exp;
};

DEFINE_EXPRESSION_WITH_PRODUCER(selection_statement, Tape) { IfElse if_else; };

void populate_if_else(SemanticAnalyzer *analyzer, IfElse *if_else,
                      const SyntaxTree *stree);
void delete_if_else(SemanticAnalyzer *analyzer, IfElse *if_else);
int produce_if_else(SemanticAnalyzer *analyzer, IfElse *if_else, Tape *tape);

DEFINE_EXPRESSION_WITH_PRODUCER(jump_statement, Tape) {
  const Token *return_token;
  ExpressionTree *exp;
};

DEFINE_EXPRESSION_WITH_PRODUCER(break_statement, Tape) {
  enum { Break_break, Break_continue } type;
  const Token *token;
};

DEFINE_EXPRESSION_WITH_PRODUCER(exit_statement, Tape) {
  const Token *token;
  ExpressionTree *value;
};

DEFINE_EXPRESSION_WITH_PRODUCER(foreach_statement, Tape) {
  Token *for_token, *in_token;
  Assignment assignment_lhs;
  ExpressionTree *iterable;
  ExpressionTree *body;
};

DEFINE_EXPRESSION_WITH_PRODUCER(for_statement, Tape) {
  Token *for_token;
  ExpressionTree *init;
  ExpressionTree *condition;
  ExpressionTree *inc;
  ExpressionTree *body;
};

DEFINE_EXPRESSION_WITH_PRODUCER(while_statement, Tape) {
  Token *while_token;
  ExpressionTree *condition;
  ExpressionTree *body;
};

typedef struct {
  const Token *module;
  const Token *token;
} ClassName;

typedef struct {
  ClassName name;
  AList *parent_classes;
} ClassSignature;

typedef struct {
  const Token *name;
  const Token *field_token;
} FieldDef;

typedef struct {
  const Token *name;
  const Token *static_token;
  ExpressionTree *value;
} StaticDef;

void delete_annotation(SemanticAnalyzer *analyzer, Annotation *annot);

typedef struct {
  ClassSignature def;
  AList *annots;
  AList *fields;
  AList *statics;
  bool has_constructor;
  FunctionDef constructor;
  AList *methods; // Function.
} ClassDef;

ClassDef populate_class(SemanticAnalyzer *analyzer, const SyntaxTree *stree);
int produce_class(SemanticAnalyzer *analyzer, ClassDef *class, Tape *tape);
void delete_class(SemanticAnalyzer *analyzer, ClassDef *class);

typedef void (*FuncDefPopulator)(const SyntaxTree *fn_identifier,
                                 FunctionDef *func);
typedef Arguments (*FuncArgumentsPopulator)(SemanticAnalyzer *analyzer,
                                            const SyntaxTree *fn_identifier,
                                            const Token *token);

void add_arg(Arguments *args, Argument *arg);
Arguments set_function_args(SemanticAnalyzer *analyzer, const SyntaxTree *stree,
                            const Token *token);
void populate_function_qualifiers(const SyntaxTree *fn_qualifiers,
                                  bool *is_const, const Token **const_token,
                                  bool *is_async, const Token **async_token);
int produce_function(SemanticAnalyzer *analyzer, FunctionDef *func, Tape *tape);
void delete_function(SemanticAnalyzer *analyzer, FunctionDef *func);
void delete_arguments(SemanticAnalyzer *analyzer, Arguments *args);
int produce_arguments(SemanticAnalyzer *analyzer, Arguments *args, Tape *tape);

typedef struct {
  bool is_named;
  Token *module_token;
  Token *module_name;
} ModuleName;

typedef struct {
  Token *import_token;
  Token *module_name;
} Import;

typedef struct {
  ModuleName name;
  AList *imports;
  AList *classes;
  AList *functions;
  AList *statements;
} ModuleDef;

DEFINE_EXPRESSION_WITH_PRODUCER(file_level_statement_list, Tape) {
  ModuleDef def;
};

#endif /* LANG_SEMANTIC_ANALYZER_DEFINITIONS_H_ */