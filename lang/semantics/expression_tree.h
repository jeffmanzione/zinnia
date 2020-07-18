// expression_tree.h
//
// Created on: Dec 27, 2019
//     Author: Jeff

#ifndef LANG_SEMANTICS_EXPRESSION_TREE_H_
#define LANG_SEMANTICS_EXPRESSION_TREE_H_

#include "lang/semantics/expression_macros.h"
#include "lang/semantics/expressions/assignment.h"
#include "lang/semantics/expressions/classes.h"
#include "lang/semantics/expressions/expression.h"
#include "lang/semantics/expressions/files.h"
#include "lang/semantics/expressions/loops.h"
#include "lang/semantics/expressions/statements.h"

struct ExpressionTree_ {
  ParseExpression type;
  union {
    Expression_identifier identifier;
    Expression_constant constant;
    Expression_string_literal string_literal;
    Expression_tuple_expression tuple_expression;
    Expression_array_declaration array_declaration;
    Expression_map_declaration map_declaration;
    Expression_primary_expression primary_expression;
    Expression_postfix_expression postfix_expression;
    Expression_range_expression range_expression;
    Expression_unary_expression unary_expression;
    Expression_multiplicative_expression multiplicative_expression;
    Expression_additive_expression additive_expression;
    Expression_relational_expression relational_expression;
    Expression_equality_expression equality_expression;
    Expression_and_expression and_expression;
    Expression_xor_expression xor_expression;
    Expression_or_expression or_expression;
    Expression_in_expression in_expression;
    Expression_is_expression is_expression;
    Expression_conditional_expression conditional_expression;
    Expression_anon_function_definition anon_function_definition;
    Expression_assignment_expression assignment_expression;
    Expression_foreach_statement foreach_statement;
    Expression_for_statement for_statement;
    Expression_while_statement while_statement;
    Expression_compound_statement compound_statement;
    Expression_try_statement try_statement;
    Expression_raise_statement raise_statement;
    Expression_selection_statement selection_statement;
    Expression_jump_statement jump_statement;
    Expression_break_statement break_statement;
    Expression_exit_statement exit_statement;
    Expression_file_level_statement_list file_level_statement_list;
  };
};

#endif /* LANG_SEMANTICS_EXPRESSION_TREE_H_ */