// string_util.c
//
// Created on: Jan 7, 2021
//     Author: Jeff Manzione

#include "util/string_util.h"

#include <stdbool.h>

#include "alloc/alloc.h"

#define DEFAULT_ESCAPED_STRING_SZ 32

bool _should_escape(char c) {
  switch (c) {
  case '\'':
  case '\"':
  case '\n':
  case '\r':
  case '\\':
    return true;
  default:
    return false;
  }
}

char _escape_char(char c) {
  switch (c) {
  case '\n':
    return 'n';
  case '\t':
    return 't';
  case '\r':
    return 'r';
  case '\\':
    return '\\';
  default:
    return c;
  }
}

int escape(const char str[], char **target) {
  if (NULL == str) {
    return 0;
  }
  const char *ptr = str;
  char c;
  char *escaped_str = MNEW_ARR(char, DEFAULT_ESCAPED_STRING_SZ);
  int escaped_len = 0, escaped_buffer_sz = DEFAULT_ESCAPED_STRING_SZ;
  while ('\0' != (c = *ptr)) {
    if (escaped_len > escaped_buffer_sz - 3) {
      escaped_str = REALLOC(escaped_str, char,
                            (escaped_buffer_sz += DEFAULT_ESCAPED_STRING_SZ));
    }
    if (_should_escape(c)) {
      escaped_str[escaped_len++] = '\\';
    }
    escaped_str[escaped_len++] = _escape_char(c);
    ptr++;
  }
  escaped_str[escaped_len] = '\0';
  escaped_str = REALLOC(escaped_str, char, escaped_len + 1);
  *target = escaped_str;
  return escaped_len;
}

char _unescape_char(char c) {
  switch (c) {
  case 'n':
    return '\n';
  case 't':
    return '\t';
  case 'r':
    return '\r';
  case '\\':
    return '\\';
  default:
    return c;
  }
}

int unescape(const char str[], char **target) {
  if (NULL == str) {
    return 0;
  }
  const char *ptr = str;
  char c;
  char *unescaped_str = MNEW_ARR(char, DEFAULT_ESCAPED_STRING_SZ);
  int unescaped_len = 0, unescaped_buffer_sz = DEFAULT_ESCAPED_STRING_SZ;
  while ('\0' != (c = *ptr)) {
    if (unescaped_len > unescaped_buffer_sz - 3) {
      unescaped_str =
          REALLOC(unescaped_str, char,
                  (unescaped_buffer_sz += DEFAULT_ESCAPED_STRING_SZ));
    }
    if (c == '\\') {
      ptr++;
      unescaped_str[unescaped_len++] = _unescape_char(*ptr);
    } else {
      unescaped_str[unescaped_len++] = c;
    }
    ptr++;
  }
  unescaped_str[unescaped_len] = '\0';
  unescaped_str = REALLOC(unescaped_str, char, unescaped_len + 1);
  *target = unescaped_str;
  return unescaped_len;
}