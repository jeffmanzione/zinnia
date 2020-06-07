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

typedef struct {
  const Token *def_token;
  const Token *fn_name;
  const Token *const_token;
  bool has_args, is_const;
  Arguments args;
  ExpressionTree *body;
} Function;

typedef void (*FuncDefPopulator)(const SyntaxTree *fn_identifier,
                                 Function *func);
typedef Arguments (*FuncArgumentsPopulator)(const SyntaxTree *fn_identifier,
                                            const Token *token);

void add_arg(Arguments *args, Argument *arg);
void set_function_def(const SyntaxTree *fn_identifier, Function *func);
Arguments set_function_args(const SyntaxTree *stree, const Token *token);

int produce_function(Function *func, Tape *tape);
void delete_function(Function *func);

Function populate_function_variant(const SyntaxTree *stree, ParseExpression def,
                                   ParseExpression signature_const,
                                   ParseExpression signature_nonconst,
                                   ParseExpression fn_identifier,
                                   ParseExpression function_arguments_no_args,
                                   ParseExpression function_arguments_present,
                                   FuncDefPopulator def_populator,
                                   FuncArgumentsPopulator args_populator);
Function populate_function(const SyntaxTree *stree);

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
