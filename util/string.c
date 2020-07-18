// string.h
//
// Created on: Jun 6, 2020
//     Author: Jeff Manzione

#include "util/string.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

bool ends_with(const char *str, const char *suffix) {
  if (!str || !suffix) {
    return false;
  }
  size_t lenstr = strlen(str);
  size_t lensuffix = strlen(suffix);
  if (lensuffix > lenstr) {
    return false;
  }
  return 0 == strncmp(str + lenstr - lensuffix, suffix, lensuffix);
}

bool starts_with(const char *str, const char *prefix) {
  if (!str || !prefix) {
    return false;
  }
  size_t lenstr = strlen(str);
  size_t lenprefix = strlen(prefix);
  if (lenprefix > lenstr) {
    return false;
  }
  return 0 == strncmp(str, prefix, lenprefix);
}

bool contains_char(const char str[], char c) {
  int i;
  for (i = 0; i < strlen(str); i++) {
    if (c == str[i]) {
      return true;
    }
  }
  return false;
}
