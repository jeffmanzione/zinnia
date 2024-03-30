#include "lang/parser/parser.h"

#include "alloc/alloc.h"
#include "alloc/arena/intern.h"
#include "lang/lexer/lang_lexer.h"
#include "lang/lexer/token.h"
#include "lang/parser/lang_parser.h"
#include "struct/q.h"
#include "util/file/file_info.h"
#include "util/file/sfile.h"

int main(int argc, const char *args[]) {
  alloc_init();
  intern_init();

  FileInfo *fi =
      file_info("C:/Users/jeffr/git/zinnia/examples/module/module.zn");

  // while (true) {
  Q tokens;
  Q_init(&tokens);

  lexer_tokenize(fi, &tokens);

  Parser parser;
  parser_init(&parser, rule_file_level_statement_list,
              /*ignore_newline=*/false);

  const SyntaxTree *stree = parser_parse(&parser, &tokens);
  syntax_tree_print(stree, 0, stdout);
  printf("\n");

  Q_iter iter = Q_iterator(&tokens);
  for (; Q_has(&iter); Q_inc(&iter)) {
    Token *token = *((Token **)Q_value(&iter));
    if (token->type != TOKEN_NEWLINE) {
      printf("EXTRA TOKEN %d '%s'\n", token->type, token->text);
    }
  }

  parser_finalize(&parser);

  Q_finalize(&tokens);
  // }

  intern_finalize();
  alloc_finalize();
  return 0;
}