#include "language-tools/parser/parser.h"

#include "file-utils/file_info.h"
#include "file-utils/sfile.h"
#include "language-tools/intern.h"
#include "language-tools/lexer/token.h"
#include "zinnia/alloc/alloc.h"
#include "zinnia/lang/lexer/lang_lexer.h"
#include "zinnia/lang/parser/lang_parser.h"
#include "zinnia/util/void_array.h"

int main(int argc, const char *args[]) {
  global_string_intern_pool_init();

  FileInfo *fi =
      file_info("C:/Users/jeffr/git/zinnia/examples/module/one/one.zn");

  // while (true) {
  TokenArray tokens;
  TokenArray_init(&tokens);

  lexer_tokenize(fi, &tokens);

  Parser parser;
  parser_init(&parser, rule_file_level_statement_list,
              /*ignore_newline=*/false);

  const SyntaxTree *stree = parser_parse(&parser, &tokens);
  syntax_tree_print(stree, 0, stdout);
  printf("\n");

  TokenArrayIterator iter;
  TokenArray_iterator(&iter, &tokens);
  for (; TokenArray_has_next(&iter); TokenArray_next(&iter)) {
    const Token *token = *TokenArray_value(&iter);
    if (token->type != TOKEN_NEWLINE) {
      printf("EXTRA TOKEN %d '%s'\n", token->type, token->text);
    }
  }

  // parser_finalize(&parser);

  // TokenArray_finalize(&tokens);
  // // }

  // global_string_intern_pool_finalize();
  return 0;
}