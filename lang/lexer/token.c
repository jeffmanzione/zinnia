// token.c
//
// Created on: Jan 6, 2016
//     Author: Jeff Manzione

#include "lang/lexer/token.h"

#include <string.h>

#include "alloc/arena/arena.h"
#include "alloc/arena/intern.h"
#include "debug/debug.h"
#include "entity/primitive.h"

static ARENA_DEFINE(Token);

char *keyword_to_type[] = {
    "if",    "then",  "else",  "def",   "new",      "field",  "method",
    "class", "while", "for",   "break", "continue", "return", "as",
    "is",    "try",   "catch", "raise", "import",   "module", "exit",
    "in",    "notin", "const", "and",   "or"};

inline char *keyword(TokenType type) {
  return keyword_to_type[type - KEYWORD_TOKEN_OFFSET];
}

TokenType word_type(const char word[], int word_len) {
  int i;
  for (i = 0; i < sizeof(keyword_to_type) / sizeof(keyword_to_type[0]); i++) {
    if (word_len != strlen(keyword_to_type[i])) {
      continue;
    }
    if (0 == strncmp(word, keyword_to_type[i], word_len)) {
      return i + KEYWORD_TOKEN_OFFSET;
    }
  }
  return WORD;
}

void token_fill(Token *tok, TokenType type, int line, int col,
                const char text[]) {
  tok->type = type;
  tok->line = line;
  tok->col = col;
  tok->len = strlen(text);
  tok->text = intern(text);
}

inline Token *token_create(TokenType type, int line, int col,
                           const char text[]) {
  if (!__ARENA__Token.inited) {
    ARENA_INIT(Token);
    __ARENA__Token.inited = true;
  }
  Token *tok = ARENA_ALLOC(Token);
  token_fill(tok, type, line, col, text);
  return tok;
}

inline void token_delete(Token *token) {
  ASSERT_NOT_NULL(token);
  ARENA_DEALLOC(Token, token);
}

inline Token *token_copy(Token *tok) {
  return token_create(tok->type, tok->line, tok->col, tok->text);
}

bool is_special_char(const char c) {
  switch (c) {
    case '(':
    case ')':
    case '{':
    case '}':
    case '[':
    case ']':
    case '+':
    case '-':
    case '*':
    case '/':
    case '\\':
    case '%':
    case '&':
    case '|':
    case '^':
    case '~':
    case '!':
    case '?':
    case '@':
    case '#':
    case '<':
    case '>':
    case '=':
    case ',':
    case ':':
    case '.':
    case '\'':
      return true;
    default:
      return false;
  }
}

inline bool is_numeric(const char c) { return ('0' <= c && '9' >= c); }

inline bool is_number(const char c) { return is_numeric(c) || '.' == c; }

inline bool is_alphabetic(const char c) {
  return ('A' <= c && 'Z' >= c) || ('a' <= c && 'z' >= c);
}

inline bool is_alphanumeric(const char c) {
  return is_numeric(c) || is_alphabetic(c) || '_' == c || '$' == c;
}

bool is_any_space(const char c) {
  switch (c) {
    case ' ':
    case '\t':
    case '\n':
    case '\r':
      return true;
    default:
      return false;
  }
}

inline bool is_whitespace(const char c) {
  return ' ' == c || '\t' == c || '\r' == c;
}

char char_unesc(char u) {
  switch (u) {
    case 'a':
      return '\a';
    case 'b':
      return '\b';
    case 'f':
      return '\f';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';
    case 'v':
      return '\v';
    case '\\':
      return '\\';
    case '\'':
      return '\'';
    case '\"':
      return '\"';
    case '\?':
      return '\?';
    default:
      return u;
  }
}

void token_print(Token *tok, FILE *file) {
  ASSERT(NOT_NULL(tok), NOT_NULL(file));
  if (tok->type >= KEYWORD_TOKEN_OFFSET) {
    fprintf(file, "Token(type=%s, text='%s'", keyword(tok->type), tok->text);
  } else {
    fprintf(file, "Token(type=%d, text='%s'", tok->type, tok->text);
  }
  if (tok->col > 0 && tok->line > 0) {
    fprintf(file, ", line=%d, col=%d", tok->line, tok->col);
  }
  fprintf(file, ")");
}

Primitive token_to_primitive(const Token *tok) {
  ASSERT_NOT_NULL(tok);
  Primitive val;
  switch (tok->type) {
    case INTEGER:
      val._type = INT;
      val._int_val = (int64_t)strtoll(tok->text, NULL, 10);
      break;
    case FLOATING:
      val._type = FLOAT;
      val._float_val = strtod(tok->text, NULL);
      break;
    default:
      ERROR("Attempted to create a Value from '%s'.", tok->text);
  }
  return val;
}

void token_finalize_all() {
  if (!__ARENA__Token.inited) {
    return;
  }
  ARENA_FINALIZE(Token);
}