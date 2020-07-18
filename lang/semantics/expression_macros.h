// expression_macros.h
//
// Created on: Dec 27, 2019
//     Author: Jeff

#ifndef LANG_SEMANTICS_EXPRESSION_MACROS_H_
#define LANG_SEMANTICS_EXPRESSION_MACROS_H_

#include <stdbool.h>
#include <stdint.h>

#include "debug/debug.h"
#include "lang/lexer/token.h"
#include "lang/parser/parser.h"
#include "program/tape.h"
#include "struct/alist.h"
#include "struct/map.h"

typedef struct ExpressionTree_ ExpressionTree;
typedef ExpressionTree *(*Populator)(const SyntaxTree *tree);
typedef int (*Producer)(ExpressionTree *tree, Tape *tape);
typedef void (*EDeleter)(ExpressionTree *tree);

ExpressionTree *populate_expression(const SyntaxTree *tree);
int produce_instructions(ExpressionTree *tree, Tape *tape);
void delete_expression(ExpressionTree *tree);

#define DefineExpression(name)                                            \
  typedef struct Expression_##name##_ Expression_##name;                  \
  ExpressionTree *Populate_##name(const SyntaxTree *tree);                \
  void Transform_##name(const SyntaxTree *tree, Expression_##name *name); \
  void Delete_##name(ExpressionTree *tree);                               \
  void Delete_##name##_inner(Expression_##name *name);                    \
  int Produce_##name(ExpressionTree *tree, Tape *tape);                   \
  int Produce_##name##_inner(Expression_##name *name, Tape *tape);        \
  struct Expression_##name##_

// TODO: Using ALLOC instead of ALLOC2 because there is some assumption in the
// code about NULL/0 for ExpressionTree. Needs further investigation before
// switching to ALLOC2.
#define ImplPopulate(name, stree_input)            \
  ExpressionTree *Populate_##name(stree_input) {   \
    ExpressionTree *etree = ALLOC(ExpressionTree); \
    etree->type = name;                            \
    Transform_##name(stree, &etree->name);         \
    return etree;                                  \
  }                                                \
  void Transform_##name(stree_input, Expression_##name *name)

#define ImplProduce(name, tape)                       \
  int Produce_##name(ExpressionTree *tree, Tape *t) { \
    return Produce_##name##_inner(&tree->name, t);    \
  }                                                   \
  int Produce_##name##_inner(Expression_##name *name, tape)

#define ImplDelete(name)                     \
  void Delete_##name(ExpressionTree *tree) { \
    Delete_##name##_inner(&tree->name);      \
  }                                          \
  void Delete_##name##_inner(Expression_##name *name)

#define EXPECT_TYPE(stree, type)   \
  if (stree->expression != type) { \
    ERROR("Expected type" #type);  \
  }

#define IS_SYNTAX(var_name, type) ((var_name->expression) == (type))

#define IS_EXPRESSION(var_name, test_type) ((var_name->type) == (test_type))

#define DECLARE_IF_TYPE(name, type, stree)    \
  SyntaxTree *name;                           \
  {                                           \
    if (stree->expression != type) {          \
      ERROR("Expected " #type " for " #name); \
      name = NULL;                            \
    } else {                                  \
      name = stree;                           \
    }                                         \
  }

#define ASSIGN_IF_TYPE(name, type, stree)     \
  {                                           \
    if (stree->expression != type) {          \
      ERROR("Expected " #type " for " #name); \
    } else {                                  \
      name = stree;                           \
    }                                         \
  }

#define IF_ASSIGN(name, type, stree, statement) \
  {                                             \
    ExpressionTree *name;                       \
    if ((stree)->expression == (type)) {        \
      name = (stree);                           \
      statement                                 \
    }                                           \
  }

#define IS_LEAF(tree) ((tree)->token != NULL)
#define IS_TOKEN(tree, token_type) \
  (IS_LEAF(tree) && ((tree)->token->type == (token_type)))

ExpressionTree *__extract_tree(AList *alist_of_tree, int index);

#define EXTRACT_TREE(alist_of_tree, i) __extract_tree(alist_of_tree, i)

#define APPEND_TREE(alist_of_tree, stree)              \
  {                                                    \
    ExpressionTree *expr = populate_expression(stree); \
    alist_append(alist_of_tree, (void *)&expr);        \
  }

#define Register(name)                              \
  {                                                 \
    map_insert(&populators, name, Populate_##name); \
    map_insert(&producers, name, Produce_##name);   \
    map_insert(&deleters, name, Delete_##name);     \
  }

#endif /* LANG_SEMANTICS_EXPRESSION_MACROS_H_ */
