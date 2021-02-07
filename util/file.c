// file.c
//
// Created on: Jun 6, 2020
//     Author: Jeff Manzione

#include "util/file.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "alloc/alloc.h"
#include "alloc/arena/intern.h"
#include "util/string.h"

#ifdef _WIN32
#define F_OK 0
#include <io.h>
#define ssize_t int
#else
#include <unistd.h>
#endif

char *find_file_by_name(const char dir[], const char file_prefix[]) {
  size_t fn_len = strlen(dir) + strlen(file_prefix) + 3;
  if (!ends_with(dir, "/")) {
    fn_len++;
  }
  char *fn = ALLOC_ARRAY2(char, fn_len);
  char *pos = fn;
  strcpy(pos, dir);
  pos += strlen(dir);
  if (!ends_with(dir, "/")) {
    strcpy(pos++, "/");
  }
  strcpy(pos, file_prefix);
  pos += strlen(file_prefix);
  strcpy(pos, ".jb");
  if (access(fn, F_OK) != -1) {
    char *to_return = intern(fn);
    DEALLOC(fn);
    return to_return;
  }
  strcpy(pos, ".ja");
  if (access(fn, F_OK) != -1) {
    char *to_return = intern(fn);
    DEALLOC(fn);
    return to_return;
  }
  strcpy(pos, ".jv");
  if (access(fn, F_OK) != -1) {
    char *to_return = intern(fn);
    DEALLOC(fn);
    return to_return;
  }
  DEALLOC(fn);
  return NULL;
}
