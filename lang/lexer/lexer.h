// lexer.h
//
// Created on: Jan 6, 2016
//     Author: Jeff Manzione

#ifndef TOKENIZER_H_
#define TOKENIZER_H_

#include <stdbool.h>

#include "struct/q.h"
#include "util/file/file_info.h"


#define CODE_DELIM " \t"
#define CODE_COMMENT_CH ';'
#define MAX_LINE_LEN 1000
#define ID_SZ 256

typedef struct {
  bool _escape_characters;
  FileInfo *_fi;
  Q _q;
} Lexer;

void lexer_init(Lexer *lexer, FileInfo *fi, bool escape_characters);
void lexer_finalize(Lexer *lexer);
FileInfo *lexer_file(Lexer *lexer);
Q *lexer_tokens(Lexer *kexer);

Q *lex(Lexer *lex);
bool lex_line(Lexer *lexer);

#endif /* TOKENIZER_H_ */
