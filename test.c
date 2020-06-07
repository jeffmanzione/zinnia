
#include <stdlib.h>

#include "alloc/alloc.h"
#include "alloc/arena/intern.h"
#include "lang/lexer/file_info.h"
#include "lang/lexer/lexer.h"
#include "lang/parser/parser.h"
#include "program/instruction.h"
#include "program/op.h"
#include "program/tape.h"

int main(int arc, char *args[]) {
  alloc_init();
  intern_init();
  parsers_init();

  Tape *tape = tape_create();
  tape_start_class(tape, intern("Test"));
  tape_start_func(tape, intern("new"));

  Instruction *i = tape_add(tape);
  i->op = LET;
  i->type = INSTRUCTION_ID;
  i->id = intern("x");

  SourceMapping *sm = tape_add_source(tape, i);
  sm->line = 1;
  sm->col = 2;

  i = tape_add(tape);
  i->op = RET;
  i->type = INSTRUCTION_NO_ARG;

  sm = tape_add_source(tape, i);
  sm->line = 2;
  sm->col = 2;

  tape_end_class(tape);

  tape_write(tape, stdout);

  FILE *tmp = tmpfile();
  fprintf(tmp, "class Test\n@new\n  let  x\n  ret\nendclass\n");
  rewind(tmp);
  FileInfo *fi = file_info_file(tmp);

  Lexer lexer;
  lexer_init(&lexer, fi, /*excape_characters*/ true);
  Q *q = lex(&lexer);

  Q_iter iter = Q_iterator(q);
  for (; Q_has(&iter); Q_inc(&iter)) {
    Token *tok = (Token *)Q_value(&iter);
    token_print(tok, stdout);
    printf("\n");
    fflush(stdout);
  }

  lexer_finalize(&lexer);
  file_info_delete(fi);

  tmp = tmpfile();
  fprintf(tmp, "module test\nclass Test {\n  new(x) {\n    return x\n}\n}\n");
  rewind(tmp);
  fi = file_info_file(tmp);

  SyntaxTree stree = parse_file(fi);
  syntax_tree_delete(&stree);

  file_info_delete(fi);

  parsers_finalize();
  intern_finalize();
  alloc_finalize();

  return EXIT_SUCCESS;
}