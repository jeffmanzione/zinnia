// semantics.c
//
// Created on: Dec 27, 2019
//     Author: Jeff Manzione

#include "lang/semantics/expression_macros.h"
#include "lang/semantics/expression_tree.h"
#include "lang/semantics/expressions/assignment.h"
#include "lang/semantics/expressions/classes.h"
#include "lang/semantics/expressions/expression.h"
#include "lang/semantics/expressions/files.h"
#include "lang/semantics/expressions/loops.h"
#include "lang/semantics/expressions/statements.h"
#include "struct/struct_defaults.h"

static Map populators;
static Map producers;
static Map deleters;

void semantics_init() {
  map_init_default(&populators);
  map_init_default(&producers);
  map_init_default(&deleters);

  Register(identifier);
  Register(constant);
  Register(string_literal);
  Register(tuple_expression);
  Register(array_declaration);
  Register(map_declaration);
  Register(primary_expression);
  Register(postfix_expression);
  Register(range_expression);
  Register(unary_expression);
  Register(multiplicative_expression);
  Register(additive_expression);
  Register(relational_expression);
  Register(equality_expression);
  Register(and_expression);
  Register(xor_expression);
  Register(or_expression);
  Register(in_expression);
  Register(is_expression);
  Register(conditional_expression);
  Register(anon_function_definition);
  Register(assignment_expression);

  Register(foreach_statement);
  Register(for_statement);
  Register(while_statement);

  Register(compound_statement);
  Register(try_statement);
  Register(raise_statement);
  Register(selection_statement);
  Register(jump_statement);
  Register(break_statement);
  Register(exit_statement);

  Register(file_level_statement_list);
}

void semantics_finalize() {
  map_finalize(&populators);
  map_finalize(&producers);
  map_finalize(&deleters);
}

ExpressionTree *populate_expression(const SyntaxTree *tree) {
  Populator populate = (Populator)map_lookup(&populators, tree->expression);
  if (NULL == populate) {
    ERROR("Populator not found: %s",
          map_lookup(&parse_expressions, tree->expression));
  }
  return populate(tree);
}

int produce_instructions(ExpressionTree *tree, Tape *tape) {
  Producer produce = (Producer)map_lookup(&producers, tree->type);
  if (NULL == produce) {
    ERROR("Producer not found.");
  }
  return produce(tree, tape);
}

void delete_expression(ExpressionTree *tree) {
  EDeleter delete = (EDeleter)map_lookup(&deleters, tree->type);
  if (NULL == delete) {
    ERROR("Deleter not found: %s", map_lookup(&parse_expressions, tree->type));
  }
  delete (tree);
  DEALLOC(tree);
}