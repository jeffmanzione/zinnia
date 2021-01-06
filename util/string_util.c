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
    return true;
  default:
    return false;
  }
}

char *escape(const char str[]) {
  if (NULL == str) {
    return NULL;
  }
  const char *ptr = str;
  char c;
  char *escaped_str = ALLOC_ARRAY2(char, DEFAULT_ESCAPED_STRING_SZ);
  int escaped_len = 0, escaped_buffer_sz = DEFAULT_ESCAPED_STRING_SZ;
  while ('\0' != (c = *ptr)) {
    if (escaped_len > escaped_buffer_sz - 3) {
      escaped_str = REALLOC(escaped_str, char,
                            (escaped_buffer_sz += DEFAULT_ESCAPED_STRING_SZ));
    }
    if (_should_escape(c)) {
      escaped_str[escaped_len++] = '\\';
    }
    escaped_str[escaped_len++] = c;
    ptr++;
  }
  escaped_str[escaped_len] = '\0';
  return REALLOC(escaped_str, char, escaped_len + 1);
}