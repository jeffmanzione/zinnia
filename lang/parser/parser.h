#ifndef LANG_PARSER_PARSER_H_
#define LANG_PARSER_PARSER_H_

#include <stdbool.h>
#include <stdio.h>

#include "lang/lexer/lexer.h"
#include "lang/lexer/token.h"
#include "struct/map.h"
#include "struct/q.h"
#include "util/file/file_info.h"

typedef struct _Parser Parser;
typedef struct _SyntaxTree SyntaxTree;
typedef SyntaxTree (*ParseExpression)(Parser *);

struct _SyntaxTree {
  bool matched;
  Token *token;
  // Queue child_exps;
  SyntaxTree *first, *second;
  ParseExpression expression;
};

struct _Parser {
  Lexer lexer;
  FileInfo *fi;
  Map *exp_names;
};

extern Map parse_expressions;

void parsers_init();
void parsers_finalize();

void parser_init(Parser *parser, FileInfo *src);
bool parser_finalize(Parser *parser);
SyntaxTree parse_file(FileInfo *src);

void syntax_tree_delete(SyntaxTree *exp);
void syntax_tree_to_str(SyntaxTree *exp, Parser *parser, FILE *file);

#define DefineSyntax(name) SyntaxTree name(Parser *parser)

DefineSyntax(identifier);
DefineSyntax(constant);
DefineSyntax(string_literal);
DefineSyntax(array_declaration);
DefineSyntax(map_declaration);
DefineSyntax(map_declaration_list);
DefineSyntax(map_declaration_entry);
DefineSyntax(map_declaration_entry1);
DefineSyntax(length_expression);
DefineSyntax(primary_expression);
DefineSyntax(primary_expression_no_constants);
DefineSyntax(postfix_expression);
DefineSyntax(postfix_expression1);
DefineSyntax(unary_expression);
DefineSyntax(multiplicative_expression);
DefineSyntax(additive_expression);
DefineSyntax(in_expression);
DefineSyntax(relational_expression);
DefineSyntax(equality_expression);
DefineSyntax(and_expression);
DefineSyntax(xor_expression);
DefineSyntax(or_expression);
DefineSyntax(multiplicative_expression1);
DefineSyntax(additive_expression1);
DefineSyntax(relational_expression1);
DefineSyntax(equality_expression1);
DefineSyntax(and_expression1);
DefineSyntax(xor_expression1);
DefineSyntax(or_expression1);
DefineSyntax(is_expression);
DefineSyntax(conditional_expression);

DefineSyntax(assignment_expression);
DefineSyntax(assignment_tuple);

DefineSyntax(primary_expression_no_constants);
DefineSyntax(const_assignment_expression);
DefineSyntax(array_index_assignment);
DefineSyntax(function_call_args);
DefineSyntax(field_expression);
DefineSyntax(field_set_value);
DefineSyntax(field_suffix);
DefineSyntax(field_next);
DefineSyntax(field_expression1);
DefineSyntax(field_expression);
DefineSyntax(assignment_lhs_single);
DefineSyntax(assignment_tuple_list);
DefineSyntax(assignment_tuple_list1);
DefineSyntax(assignment_array);
DefineSyntax(assignment_lhs);
DefineSyntax(catch_assign);
DefineSyntax(class_compound_statement);
DefineSyntax(class_name_and_inheritance);
DefineSyntax(parent_classes);

DefineSyntax(function_arg_default_value);
DefineSyntax(function_arg_elt_with_default);
DefineSyntax(function_arg_elt);
DefineSyntax(const_function_argument);
DefineSyntax(function_argment);
DefineSyntax(function_argument_list1);
DefineSyntax(function_argument_list);
DefineSyntax(function_arguments);
DefineSyntax(function_definition);
DefineSyntax(def_identifier);
DefineSyntax(function_signature_no_qualifier);
DefineSyntax(function_qualifier);
DefineSyntax(function_qualifier_list1);
DefineSyntax(function_qualifier_list);
DefineSyntax(function_signature_with_qualifier);
DefineSyntax(function_signature);
DefineSyntax(function_arguments_present);
DefineSyntax(function_arguments_no_args);

DefineSyntax(method_identifier);
DefineSyntax(method_signature_no_qualifier);
DefineSyntax(method_signature_with_qualifier);
DefineSyntax(method_signature);
DefineSyntax(method_definition);

DefineSyntax(new_arg_var);
DefineSyntax(new_field_arg);
DefineSyntax(new_arg_default_value);
DefineSyntax(new_arg_elt_with_default);
DefineSyntax(new_arg_elt);
DefineSyntax(const_new_argument);
DefineSyntax(new_argument);
DefineSyntax(new_argument_list1);
DefineSyntax(new_argument_list);
DefineSyntax(new_arguments_present);
DefineSyntax(new_arguments_no_args);
DefineSyntax(new_arguments);
DefineSyntax(new_identifier);
DefineSyntax(new_expression);
DefineSyntax(new_signature_nonconst);
DefineSyntax(new_signature_const);
DefineSyntax(new_signature);
DefineSyntax(new_definition);

DefineSyntax(anon_identifier);
DefineSyntax(anon_signature_no_qualifier);
DefineSyntax(anon_signature_with_qualifier);
DefineSyntax(anon_signature);
DefineSyntax(anon_function_definition);
DefineSyntax(anon_function_lambda_rhs);

DefineSyntax(tuple_expression);
DefineSyntax(tuple_expression1);
DefineSyntax(expression_statement);
DefineSyntax(iteration_statement);
DefineSyntax(compound_statement);
DefineSyntax(selection_statement);
DefineSyntax(jump_statement);
DefineSyntax(function_argument_list);
DefineSyntax(function_argument_list1);
DefineSyntax(function_definition);
DefineSyntax(method_definition);
DefineSyntax(identifier_list);
DefineSyntax(identifier_list1);
DefineSyntax(field_statement);
DefineSyntax(annotation);
DefineSyntax(annotation_not_called);
DefineSyntax(annotation_no_arguments);
DefineSyntax(annotation_with_arguments);
DefineSyntax(class_definition_no_annotation);
DefineSyntax(class_definition_with_annotation);
DefineSyntax(class_statement);
DefineSyntax(class_statement_list);
DefineSyntax(class_statement_list1);
DefineSyntax(import_statement);
DefineSyntax(module_statement);
DefineSyntax(statement);
DefineSyntax(statement_list);
DefineSyntax(statement_list1);
DefineSyntax(file_level_statement);
DefineSyntax(file_level_statement_list);
DefineSyntax(file_level_statement_list1);
DefineSyntax(new_definition);
DefineSyntax(parent_class_list);
DefineSyntax(parent_class_list1);
DefineSyntax(break_statement);
DefineSyntax(try_statement);
DefineSyntax(exit_statement);
DefineSyntax(raise_statement);
DefineSyntax(range_expression);
DefineSyntax(foreach_statement);
DefineSyntax(for_statement);
DefineSyntax(while_statement);
DefineSyntax(const_expression);
DefineSyntax(default_value_expression);

#endif /* LANG_PARSER_PARSER_H_ */