// files.h
//
// Created on: Dec 29, 2019
//     Author: Jeff Manzione

#ifndef LANG_SEMANTICS_EXPRESSIONS_FILES_H_
#define LANG_SEMANTICS_EXPRESSIONS_FILES_H_

#include "lang/lexer/token.h"
#include "lang/semantics/expression_macros.h"
#include "lang/semantics/expressions/assignment.h"
#include "struct/alist.h"

typedef struct {
  bool is_const, is_field, has_default;
  const Token *arg_name;
  const Token *const_token;
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

typedef void (*FuncDefPopulator)(const SyntaxTree *fn_identifier,
                                 FunctionDef *func);
typedef Arguments (*FuncArgumentsPopulator)(const SyntaxTree *fn_identifier,
                                            const Token *token);

void add_arg(Arguments *args, Argument *arg);
void set_function_def(const SyntaxTree *fn_identifier, FunctionDef *func);
Arguments set_function_args(const SyntaxTree *stree, const Token *token);

void populate_function_qualifiers(const SyntaxTree *fn_qualifiers, bool *is_const, const Token **const_token, bool *is_async, const Token **async_token);
int produce_function(FunctionDef *func, Tape *tape);
void delete_function(FunctionDef *func);

FunctionDef populate_function_variant(
    const SyntaxTree *stree, ParseExpression def,
    ParseExpression signature_with_qualifier, ParseExpression signature_no_qualifier,
    ParseExpression fn_identifier, ParseExpression function_arguments_no_args,
    ParseExpression function_arguments_present, FuncDefPopulator def_populator,
    FuncArgumentsPopulator args_populator);
FunctionDef populate_function(const SyntaxTree *stree);

int produce_arguments(Arguments *args, Tape *tape);

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

DefineExpression(file_level_statement_list) { ModuleDef def; };

#endif /* LANG_SEMANTICS_EXPRESSIONS_FILES_H_ */
