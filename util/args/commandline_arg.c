// commandline_arg.c
//
// Created on: Jun 2, 2018
//     Author: Jeff

#include "util/args/commandline_arg.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "alloc/alloc.h"
#include "alloc/arena/intern.h"
#include "debug/debug.h"

bool string_is_true(const char str[]) {
  if (0 == strcmp("True", str) || 0 == strcmp("true", str) ||
      0 == strncmp("1", str, 1)) {
    return true;
  }
  return false;
}

bool parse_bool(const char val[], bool *bool_val) {
  *bool_val = string_is_true(val);
  return true;
}

bool parse_int(const char val[], int32_t *int_val) {
  char *pEnd;
  long int proto_val = strtol(val, &pEnd, 10);
  if (0 == proto_val && errno != 0) {
    return false;
  }
  if (pEnd != val + strlen(val)) {
    return false;
  }
  *int_val = (int32_t)proto_val;
  return true;
}

bool parse_float(const char val[], float *float_val) {
  char *pEnd;
  double proto_val = strtod(val, &pEnd);
  if (0 == proto_val && errno != 0) {
    return false;
  }
  if (pEnd != val + strlen(val)) {
    return false;
  }
  *float_val = (float)proto_val;
  return true;
}

bool parse_stringlist(const char val[], char ***stringlist_val, int *count) {
  int comma_count = 0;
  char *pos = (char *)val;
  while (NULL != (pos = strchr(pos, ','))) {
    comma_count++;
    pos++;
  }
  *count = comma_count + 1;
  *stringlist_val = MNEW_ARR(char *, comma_count + 1);

  char *prev = (char *)val;
  pos = (char *)val;
  int index = 0;
  while (NULL != (pos = strchr(prev, ','))) {
    (*stringlist_val)[index++] = intern_range(prev, 0, pos - prev);
    prev = pos + 1;
  }
  (*stringlist_val)[index] = intern(prev);
  return true;
}

Arg arg_parse(ArgType type, const char val[]) {
  Arg arg;
  arg.used = true;
  arg.type = type;
  switch (type) {
  case ArgType__string:
    arg.string_val = val;
    break;
  case ArgType__bool:
    if (!parse_bool(val, &arg.bool_val)) {
      FATALF("Could not parse '%s' to BOOL.", val);
    }
    break;
  case ArgType__int:
    if (!parse_int(val, &arg.int_val)) {
      FATALF("Could not parse '%s' to INT.", val);
    }
    break;
  case ArgType__float:
    if (!parse_float(val, &arg.float_val)) {
      FATALF("Could not parse '%s' to FLOAT.", val);
    }
    break;
  case ArgType__stringlist:
    if (!parse_stringlist(val, &arg.stringlist_val, &arg.count)) {
      FATALF("Could not parse '%s' to STRING LIST.", val);
    }
    break;
  default:
    FATALF("ArgType not specified.");
  }
  return arg;
}

Arg arg_bool(bool bool_val) {
  Arg arg = {.used = true, .type = ArgType__bool, .bool_val = bool_val};
  return arg;
}

Arg arg_int(int32_t int_val) {
  Arg arg = {.used = true, .type = ArgType__int, .int_val = int_val};
  return arg;
}

Arg arg_float(float float_val) {
  Arg arg = {.used = true, .type = ArgType__float, .float_val = float_val};
  return arg;
}

Arg arg_string(const char string_val[]) {
  Arg arg = {.used = true, .type = ArgType__string, .string_val = string_val};
  return arg;
}

Arg arg_stringlist(const char string_val[]) {
  Arg arg = {.used = true, .type = ArgType__stringlist};
  if (!parse_stringlist(string_val, &arg.stringlist_val, &arg.count)) {
    FATALF("Could not parse '%s' to STRING LIST.", string_val);
  }
  return arg;
}