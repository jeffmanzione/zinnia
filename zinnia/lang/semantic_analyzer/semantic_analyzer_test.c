#include "language-tools/semantic_analyzer/semantic_analyzer.h"

#include "file-utils/file_info.h"
#include "file-utils/sfile.h"
#include "language-tools/parser/parser.h"
#include "language-tools/semantic_analyzer/expression_tree.h"
#include "zinnia/alloc/alloc.h"
#include "zinnia/lang/lexer/lang_lexer.h"
#include "zinnia/lang/parser/lang_parser.h"
#include "zinnia/lang/semantic_analyzer/definitions.h"
#include "zinnia/program/tape.h"
#include "zinnia/util/error.h"
#include "zinnia/util/void_array.h"
#include "zinnia/vm/intern.h" #include "language-tools/lexer/token.h"
#include "zinnia/vm/intern.h"
#include "zinnia/vm/module_manager.h"


int main(int argc, const char *args[]) {
  global_string_intern_pool_init();
  strings_init();

  const char *fn = args[1];

  FileInfo *fi = file_info(fn);

  // while (true) {
  TokenArray tokens;
  TokenArray_init(&tokens);

  // printf("> ");
  lexer_tokenize(fi, &tokens);

  Parser parser;
  parser_init(&parser, rule_file_level_statement_list,
              /*ignore_newline=*/false);
  SyntaxTree *stree = parser_parse(&parser, &tokens);
  stree = parser_prune_newlines(&parser, stree);
  syntax_tree_print(stree, 0, stdout);
  printf("\n");

  if (TokenArray_size(&tokens) > 1) {
    fatal_on_token(fn, fi, &tokens);
    return -1;
  } else {
    SemanticAnalyzer sa;
    semantic_analyzer_init(&sa, semantic_analyzer_init_fn);
    ExpressionTree *etree = semantic_analyzer_populate(&sa, stree);

    Tape *tape = tape_create();
    Token token = {.text = global_intern("TEST")};
    tape_module(tape, &token);
    semantic_analyzer_produce(&sa, etree, tape);
    tape_write(tape, stdout, /* minimize */ false);

    tape_delete(tape);

    semantic_analyzer_delete(&sa, etree);
    semantic_analyzer_finalize(&sa);
  }

#ifdef DEBUG

  parser_delete_st(&parser, stree);
  parser_finalize(&parser);

  TokenArray_finalize(&tokens);
  // }

  file_info_delete(fi);

  token_finalize_all();
  strings_finalize();
  global_string_intern_pool_finalize();
#endif
  return 0;
}