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
} ClassDef;

typedef struct {
  const Token *name;
  const Token *field_token;
} Field;

typedef struct {
  ClassDef def;
  AList *fields;
  bool has_constructor;
  Function constructor;
  AList *methods;  // Function.
} Class;

Class populate_class(const SyntaxTree *stree);
int produce_class(Class *class, Tape *tape);
void delete_class(Class *class);

#endif /* LANG_SEMANTICS_EXPRESSIONS_CLASSES_H_ */
