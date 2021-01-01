// lexer.c
//
// Created on: Jan 6, 2016
//     Author: Jeff Manzione

#include "lang/lexer/lexer.h"

#include <stdio.h>
#include <string.h>

#include "alloc/alloc.h"
#include "alloc/arena/arena.h"
#include "alloc/arena/intern.h"
#include "debug/debug.h"
#include "lang/lexer/token.h"
#include "struct/q.h"
#include "util/file/file_info.h"
#include "util/string.h"


TokenType _resolve_type(const char word[], int word_len, int *in_comment) {
  TokenType type;
  if (0 == word_len) {
    type = ENDLINE;
    *in_comment = false;
  } else {
    switch (word[0]) {
    case '(':
      type = LPAREN;
      break;
    case ')':
      type = RPAREN;
      break;
    case '{':
      type = LBRCE;
      break;
    case '}':
      type = RBRCE;
      break;
    case '[':
      type = LBRAC;
      break;
    case ']':
      type = RBRAC;
      break;
    case '+':
      type = word[1] == '+' ? INCREMENT : PLUS;
      break;
    case '-':
      type = word[1] == '>' ? RARROW : (word[1] == '-' ? DECREMENT : MINUS);
      break;
    case '*':
      type = STAR;
      break;
    case '/':
      type = FSLASH;
      break;
    case '\\':
      type = BSLASH;
      break;
    case '%':
      type = PERCENT;
      break;
    case '&':
      type = AMPER;
      break;
    case '|':
      type = PIPE;
      break;
    case '^':
      type = CARET;
      break;
    case '~':
      type = TILDE;
      break;
    case '!':
      type = word[1] == '=' ? NEQUIV : EXCLAIM;
      break;
    case '?':
      type = QUESTION;
      break;
    case '@':
      type = AT;
      break;
    case '#':
      type = POUND;
      break;
    case '<':
      type = word[1] == '-' ? LARROW : (word[1] == '=' ? LTHANEQ : LTHAN);
      break;
    case '>':
      type = word[1] == '=' ? GTHANEQ : GTHAN;
      break;
    case '=':
      type = word[1] == '=' ? EQUIV : EQUALS;
      break;
    case ',':
      type = COMMA;
      break;
    case ':':
      type = COLON;
      break;
    case '.':
      type = PERIOD;
      break;
    case '\'':
      type = STR;
      break;
    case CODE_COMMENT_CH:
      type = SEMICOLON;
      *in_comment = true;
      break;
    default:
      if (is_number(word[0])) {
        if (ends_with(word, "f") || contains_char(word, '.')) {
          type = FLOATING;
        } else {
          type = INTEGER;
        }
      } else {
        type = word_type(word, word_len);
      }
    }
  }
  return type;
}

bool is_complex(const char seq[]) {
  const char sec = seq[1];
  switch (seq[0]) {
  case '+':
    if (sec == '+' || sec == '=')
      return true;
    return false;
  case '-':
    if (sec == '-' || sec == '=' || sec == '>')
      return true;
    return false;
  case '<':
    if (sec == '-' || sec == '=' || sec == '<' || sec == '>')
      return true;
    return false;
  case '>':
    if (sec == '=' || sec == '>')
      return true;
    return false;
  case '=':
    if (sec == '=')
      return true;
    return false;
  case '!':
    if (sec == '=')
      return true;
    return false;
  default:
    return false;
  }
}

int _read_word(char **ptr, char word[], int *word_len) {
  char *index = *ptr, *tmp;
  int wrd_ln = 0;
  if (0 == index[0]) {
    word[0] = '\0';
    *word_len = 0;
    tmp = *ptr;
    return 0;
  }
  while (is_whitespace(index[0])) {
    index++;
  }
  if ((is_special_char(index[0]) || CODE_COMMENT_CH == index[0])) {
    if (is_complex(index)) {
      word[wrd_ln++] = index[0];
      index++;
    }
    word[wrd_ln++] = index[0];
    index++;
  } else {
    if ('\n' != index[0]) {
      word[wrd_ln++] = index[0];
      index++;

      if (is_alphanumeric(word[0])) {
        if (is_number(word[0])) {
          while (is_number(index[0]) || 'f' == index[0]) {
            word[wrd_ln++] = index[0];
            index++;
          }
        } else {
          while (is_alphanumeric(index[0])) {
            word[wrd_ln++] = index[0];
            index++;
          }
        }
      }
    }
  }
  word[wrd_ln] = '\0';
  *word_len = wrd_ln;
  tmp = *ptr;
  *ptr = index;
  return *ptr - tmp;
}

int _read_string(const char line[], char **index, char *word,
                 bool escape_characters) {
  int word_i = 1;
  word[0] = '\'';
  while ('\'' != **index) {
    if ('\\' == **index && escape_characters) {
      word[word_i++] = char_unesc(*++*index);
    } else {
      word[word_i++] = **index;
    }
    (*index)++;
  }
  word[word_i++] = **index;
  (*index)++;
  word[word_i] = '\0';
  return word_i;
}

void lexer_init(Lexer *lexer, FileInfo *fi, bool escape_characters) {
  ASSERT(NOT_NULL(lexer), NOT_NULL(fi));
  lexer->_fi = fi;
  lexer->_escape_characters = escape_characters;
  Q_init(&lexer->_q);
}

void lexer_finalize(Lexer *lexer) { Q_finalize(&lexer->_q); }

inline FileInfo *lexer_file(Lexer *lexer) {
  ASSERT(NOT_NULL(lexer), NOT_NULL(lexer->_fi));
  return lexer->_fi;
}

inline Q *lexer_tokens(Lexer *lexer) {
  ASSERT(NOT_NULL(lexer));
  return &lexer->_q;
}

bool lex_line(Lexer *lexer) {
  ASSERT(NOT_NULL(lexer));
  char word[MAX_LINE_LEN];
  char *index;
  int in_comment = false;
  TokenType type;
  int col_num = 0, word_len = 0, chars_consumed;

  LineInfo *li = file_info_getline(lexer_file(lexer));
  if (NULL == li) {
    return false;
  }

  char *line = li->line_text;

  col_num = 0;
  index = line;

  while (true) {
    chars_consumed = _read_word(&index, word, &word_len);
    type = _resolve_type(word, word_len, &in_comment);
    col_num += chars_consumed;
    if (in_comment) {
      continue;
    }
    if (STR == type) {
      _read_string(line, &index, word, lexer->_escape_characters);
    }
    if (ENDLINE == type && Q_size(&lexer->_q) > 0 &&
        Q_last(&lexer->_q) != NULL &&
        ((Token *)Q_last(&lexer->_q))->type == BSLASH) {
      Token *bslash = Q_dequeue(&lexer->_q);
      token_delete(bslash);
      lex_line(lexer);
    } else if (ENDLINE != type ||
               (Q_size(&lexer->_q) != 0 &&
                ENDLINE != ((Token *)Q_last(&lexer->_q))->type)) {
      Q_enqueue(&lexer->_q,
                token_create(type, li->line_num, col_num - strlen(word), word));
    }
    if (0 == chars_consumed) {
      break;
    }
  }
  return true;
}

Q *lex(Lexer *lexer) {
  ASSERT(NOT_NULL(lexer));
  while (lex_line(lexer))
    ;
  return &lexer->_q;
}
