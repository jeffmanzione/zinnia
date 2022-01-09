#include "lang/parser/parser.h"

#include "alloc/alloc.h"
#include "alloc/arena/intern.h"
#include "debug/debug.h"
#include "lang/lexer/lang_lexer.h"
#include "lang/lexer/token.h"
#include "lang/parser/lang_parser.h"
#include "lang/semantic_analyzer/definitions.h"
#include "lang/semantic_analyzer/expression_tree.h"
#include "lang/semantic_analyzer/semantic_analyzer.h"
#include "program/tape.h"
#include "struct/q.h"
#include "util/file/file_info.h"
#include "util/file/sfile.h"
#include "vm/intern.h"

REGISTRATION_FN(_semantic_analyzer_init_fn) {
  REGISTER_EXPRESSION_WITH_PRODUCER(identifier);
  REGISTER_EXPRESSION_WITH_PRODUCER(constant);
  REGISTER_EXPRESSION_WITH_PRODUCER(string_literal);
  REGISTER_EXPRESSION_WITH_PRODUCER(tuple_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(array_declaration);
  REGISTER_EXPRESSION_WITH_PRODUCER(primary_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(primary_expression_no_constants);
  REGISTER_EXPRESSION_WITH_PRODUCER(postfix_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(range_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(unary_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(multiplicative_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(additive_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(relational_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(equality_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(and_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(or_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(binary_and_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(binary_xor_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(binary_or_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(in_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(is_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(conditional_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(anon_function_definition);
  REGISTER_EXPRESSION_WITH_PRODUCER(map_declaration);
  REGISTER_EXPRESSION_WITH_PRODUCER(assignment_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(compound_statement);
  REGISTER_EXPRESSION_WITH_PRODUCER(try_statement);
  REGISTER_EXPRESSION_WITH_PRODUCER(raise_statement);
  REGISTER_EXPRESSION_WITH_PRODUCER(selection_statement);
  REGISTER_EXPRESSION_WITH_PRODUCER(jump_statement);
  REGISTER_EXPRESSION_WITH_PRODUCER(break_statement);
  REGISTER_EXPRESSION_WITH_PRODUCER(exit_statement);
  REGISTER_EXPRESSION_WITH_PRODUCER(foreach_statement);
  REGISTER_EXPRESSION_WITH_PRODUCER(for_statement);
  REGISTER_EXPRESSION_WITH_PRODUCER(while_statement);
  REGISTER_EXPRESSION_WITH_PRODUCER(file_level_statement_list);
}

DEFINE_SEMANTIC_ANALYZER_PRODUCE_FN(Tape);

int main(int argc, const char *args[]) {
  alloc_init();
  intern_init();
  strings_init();

  FileInfo *fi = file_info("C:\\Users\\jeffr\\git\\jeff-vm\\lib\\builtin.jv");

  // while (true) {
  Q tokens;
  Q_init(&tokens);

  // printf("> ");
  lexer_tokenize(fi, &tokens);

  Parser parser;
  parser_init(&parser, rule_file_level_statement_list,
              /*ignore_newline=*/false);
  SyntaxTree *stree = parser_parse(&parser, &tokens);
  stree = parser_prune_newlines(&parser, stree);
  syntax_tree_print(stree, 0, stdout);
  printf("\n");

  if (Q_size(&tokens) > 1) {
    Q_iter iter = Q_iterator(&tokens);
    for (; Q_has(&iter); Q_inc(&iter)) {
      Token *token = *((Token **)Q_value(&iter));
      if (token->type != TOKEN_NEWLINE) {
        printf("EXTRA TOKEN %d '%s'\n", token->type, token->text);
      }
    }
  } else {
    SemanticAnalyzer sa;
    semantic_analyzer_init(&sa, _semantic_analyzer_init_fn);
    ExpressionTree *etree = semantic_analyzer_populate(&sa, stree);

    Tape *tape = tape_create();
    Token token = {.text = intern("TEST")};
    tape_module(tape, &token);
    semantic_analyzer_produce(&sa, etree, tape);
    tape_write(tape, stdout);

    tape_delete(tape);

    semantic_analyzer_delete(&sa, etree);
    semantic_analyzer_finalize(&sa);
  }

  parser_delete_st(&parser, stree);
  parser_finalize(&parser);

  Q_finalize(&tokens);
  // }

  file_info_delete(fi);

  token_finalize_all();
  strings_finalize();
  intern_finalize();
  alloc_finalize();
  return 0;
}