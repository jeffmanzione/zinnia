// classes.h
//
// Created on: Dec 30, 2019
//     Author: Jeff Manzione

#ifndef LANG_SEMANTICS_EXPRESSIONS_CLASSES_H_
#define LANG_SEMANTICS_EXPRESSIONS_CLASSES_H_

#include <stdbool.h>

#include "lang/lexer/token.h"
#include "lang/semantics/expression_macros.h"
#include "lang/semantics/expressions/files.h"
#include "struct/alist.h"

typedef struct {
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
  ClassSignature def;
  AList *fields;
  bool has_constructor;
  FunctionDef constructor;
  AList *methods; // Function.
} ClassDef;

ClassDef populate_class(const SyntaxTree *stree);
int produce_class(ClassDef *class, Tape *tape);
void delete_class(ClassDef *class);

#endif /* LANG_SEMANTICS_EXPRESSIONS_CLASSES_H_ */
