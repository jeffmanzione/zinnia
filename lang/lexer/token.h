// token.h
//
// Created on: Jan 6, 2016
//     Author: Jeff Manzione

#ifndef LANG_LEXER_TOKEN_H_
#define LANG_LEXER_TOKEN_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "alloc/arena/arena.h"
#include "entity/primitive.h"

#define KEYWORD_TOKEN_OFFSET 1000

typedef enum {
  WORD,
  INTEGER,
  FLOATING,

  // For strings.
  STR,

  // Structural symbols
  LPAREN,
  RPAREN,
  LBRCE,
  RBRCE,
  LBRAC,
  RBRAC,

  // Math symbols
  PLUS,
  MINUS,
  STAR,
  FSLASH,
  BSLASH,
  PERCENT,

  // Binary/bool symbols
  AMPER,
  PIPE,
  CARET,
  TILDE,

  // Following not used yet
  EXCLAIM,
  QUESTION,
  AT,
  POUND,

  // Equivalence
  LTHAN,
  GTHAN,
  EQUALS,

  // Others
  ENDLINE,
  SEMICOLON,
  COMMA,
  COLON,
  PERIOD,

  // Specials
  LARROW,
  RARROW,
  INCREMENT,
  DECREMENT,
  LTHANEQ,
  GTHANEQ,
  EQUIV,
  NEQUIV,

  // Words
  IF_T = KEYWORD_TOKEN_OFFSET,
  THEN,
  ELSE,
  DEF,
  NEW,
  FIELD,
  METHOD,
  CLASS,
  WHILE,
  FOR,
  BREAK,
  CONTINUE,
  RETURN,
  AS,
  IS_T,
  TRY,
  CATCH,
  RAISE,
  IMPORT,
  MODULE_T,
  EXIT_T,
  IN,
  NOTIN,
  CONST_T,
  AND_T,
  OR_T,
} TokenType;

typedef struct {
  TokenType type;
  int col, line;
  size_t len;
  const char *text;
} Token;

char *keyword(TokenType type);
TokenType word_type(const char word[], int word_len);

bool is_number(const char c);
bool is_numeric(const char c);
bool is_alphabetic(const char c);
bool is_alphanumeric(const char c);

bool is_whitespace(const char c);
bool is_any_space(const char c);
char char_unesc(char u);

bool is_special_char(const char c);

void token_fill(Token *tok, TokenType type, int line, int col,
                const char text[]);
Token *token_create(TokenType type, int line, int col, const char text[]);
Token *token_copy(Token *tok);
void token_delete(Token *tok);

Primitive token_to_primitive(const Token *tok);
void token_print(Token *tok, FILE *file);

void token_finalize_all();

#endif /* LANG_LEXER_TOKEN_H_ */