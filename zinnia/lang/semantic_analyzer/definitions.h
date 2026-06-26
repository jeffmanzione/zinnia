#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_LANG_SEMANTIC_ANALYZER_DEFINITIONS_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_LANG_SEMANTIC_ANALYZER_DEFINITIONS_H_

#include "c-data-structures/arraylike.h"
#include "language-tools/lexer/token.h"
#include "language-tools/semantic_analyzer/expression_tree.h"
#include "language-tools/semantic_analyzer/semantic_analyzer.h"
#include "zinnia/program/tape.h"
#include "zinnia/util/error.h"

DEFINE_SEMANTIC_ANALYZER_PRODUCE_FN(Tape);

void semantic_analyzer_init_fn(SAMap *populators, SAMap *producers,
                               SAMap *deleters);

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
  ExpressionTreeArray list;
};

DEFINE_EXPRESSION_WITH_PRODUCER(array_declaration, Tape) {
  Token *token;
  bool is_empty;
  ExpressionTree *exp;  // A tuple expression.
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

DEFINE_ARRAYLIKE(NamedArgumentArray, NamedArgument);

DEFINE_EXPRESSION_WITH_PRODUCER(named_argument_list, Tape) {
  Token *token;
  // NamedArgument
  NamedArgumentArray list;
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
    ExpressionTree *exp;  // Can be NULL for empty fncall.
  };
  Token *token;
} Postfix;

DEFINE_ARRAYLIKE(PostfixArray, Postfix);

int produce_postfix(SemanticAnalyzer *analyzer, int *i, int num_postfix,
                    const PostfixArray *suffixes, const Postfix **next,
                    Tape *tape);

DEFINE_EXPRESSION_WITH_PRODUCER(postfix_expression, Tape) {
  ExpressionTree *prefix;
  PostfixArray suffixes;
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

DEFINE_ARRAYLIKE(BiSuffixArray, BiSuffix);

#define DEFINE_BI_EXPRESSION(expr)              \
  DEFINE_EXPRESSION_WITH_PRODUCER(expr, Tape) { \
    ExpressionTree *exp;                        \
    BiSuffixArray suffixes;                     \
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

DEFINE_ARRAYLIKE(ConditionalArray, Conditional);

typedef struct {
  ConditionalArray conditions;
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

DEFINE_ARRAYLIKE(ArgumentArray, Argument);

typedef struct {
  const Token *token;
  int count_required, count_optional;
  bool is_named;
  ArgumentArray args;
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

DEFINE_ARRAYLIKE(AnnotationArray, Annotation);

typedef struct {
  const Token *def_token;
  const Token *fn_name;
  SpecialMethod special_method;
  AnnotationArray annots;
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

DEFINE_ARRAYLIKE(MapDecEntryArray, MapDecEntry);

DEFINE_EXPRESSION_WITH_PRODUCER(map_declaration, Tape) {
  Token *lbrce, *rbrce;
  bool is_empty;
  MapDecEntryArray entries;
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
      PostfixArray suffixes;  // Should be Posfixes.
    };
  };
} SingleAssignment;

typedef struct AssignmentArray_ AssignmentArray;

typedef struct {
  AssignmentArray *subargs;
} MultiAssignment;

typedef struct {
  enum { Assignment_single, Assignment_array, Assignment_tuple } type;
  union {
    SingleAssignment single;
    MultiAssignment multi;
  };
} Assignment;

DEFINE_ARRAYLIKE(AssignmentArray, Assignment);

Assignment populate_assignment(SemanticAnalyzer *analyzer,
                               const SyntaxTree *stree);
void delete_assignment(SemanticAnalyzer *analyzer, Assignment *assignment);
int produce_assignment(SemanticAnalyzer *analyzer, const Assignment *assign,
                       Tape *tape, const Token *eq_token);

DEFINE_EXPRESSION_WITH_PRODUCER(assignment_expression, Tape) {
  Token *eq_token;
  Assignment assignment;
  ExpressionTree *rhs;
};

DEFINE_EXPRESSION_WITH_PRODUCER(compound_statement, Tape) {
  ExpressionTreeArray expressions;
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

DEFINE_ARRAYLIKE(ClassNameArray, ClassName);

typedef struct {
  ClassName name;
  ClassNameArray parent_classes;
} ClassSignature;

typedef struct {
  const Token *name;
  const Token *field_token;
} FieldDef;

DEFINE_ARRAYLIKE(FieldDefArray, FieldDef);

typedef struct {
  const Token *name;
  const Token *static_token;
  ExpressionTree *value;
} StaticDef;

DEFINE_ARRAYLIKE(StaticDefArray, StaticDef);

void delete_annotation(SemanticAnalyzer *analyzer, Annotation *annot);

DEFINE_ARRAYLIKE(FunctionDefArray, FunctionDef);

typedef struct {
  ClassSignature def;
  AnnotationArray annots;
  FieldDefArray fields;
  StaticDefArray statics;
  bool has_constructor;
  FunctionDef constructor;
  FunctionDefArray methods;  // Function.
} ClassDef;

ClassDef populate_class(SemanticAnalyzer *analyzer, const SyntaxTree *stree);
int produce_class(SemanticAnalyzer *analyzer, const ClassDef *class,
                  Tape *tape);
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
int produce_function(SemanticAnalyzer *analyzer, const FunctionDef *func,
                     Tape *tape);
void delete_function(SemanticAnalyzer *analyzer, FunctionDef *func);
void delete_arguments(SemanticAnalyzer *analyzer, Arguments *args);
int produce_arguments(SemanticAnalyzer *analyzer, const Arguments *args,
                      Tape *tape);

typedef struct {
  bool is_named;
  Token *module_token;
  Token *module_name;
} ModuleName;

typedef struct {
  Token *import_token;
  Token *src_name;
  Token *as_token;
  Token *use_name;
  bool is_string_import;
} Import;

DEFINE_ARRAYLIKE(ImportArray, Import);
DEFINE_ARRAYLIKE(ClassDefArray, ClassDef);

typedef struct {
  ModuleName name;
  ImportArray imports;
  ClassDefArray classes;
  FunctionDefArray functions;
  ExpressionTreeArray statements;
} ModuleDef;

DEFINE_EXPRESSION_WITH_PRODUCER(file_level_statement_list, Tape) {
  ModuleDef def;
};

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_LANG_SEMANTIC_ANALYZER_DEFINITIONS_H_*/