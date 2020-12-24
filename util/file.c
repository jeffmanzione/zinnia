// file.c
//
// Created on: Jun 6, 2020
//     Author: Jeff Manzione

#include "util/file.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#define F_OK 0
#include <io.h>
#else
#include <unistd.h>
#endif

#include "alloc/alloc.h"
#include "alloc/arena/intern.h"
#include "debug/debug.h"
#include "util/string.h"

#ifdef _WIN32
#define SLASH_CHAR '\\';
#else
#define SLASH_CHAR '/';
#endif

FILE *file_fn(const char fn[], const char op_type[], int line_num,
              const char func_name[], const char file_name[]) {
  if (NULL == fn) {
    __error(line_num, func_name, file_name, "Unable to read file.");
  }
  if (NULL == op_type) {
    __error(line_num, func_name, file_name, "Unable to read file '%s'.", fn);
  }
  FILE *file = fopen(fn, op_type);
  if (NULL == file) {
    __error(line_num, func_name, file_name,
            "Unable to read file '%s' with op '%s'.", fn, op_type);
  }
  return file;
}

void file_op(FILE *file, FileHandler operation, int line_num,
             const char func_name[], const char file_name[]) {
  operation(file);
  if (0 != fclose(file)) {
    __error(line_num, func_name, file_name, "File operation failed.");
  }
}

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
  strcpy(pos, ".jl");
  if (access(fn, F_OK) != -1) {
    char *to_return = intern(fn);
    DEALLOC(fn);
    return to_return;
  }
  DEALLOC(fn);
  return NULL;
}

void make_dir_if_does_not_exist(const char path[]) {
  ASSERT(NOT_NULL(path));
  struct stat st = {0};
  if (stat(path, &st) == -1) {
#ifdef _WIN32
    mkdir(path);
#else
    mkdir(path, 0700);
#endif
  }
}

void split_path_file(const char path_file[], char **path, char **file_name,
                     char **ext) {
  char *slash = (char *)path_file, *next;
  while ((next = strpbrk(slash + 1, "\\/"))) slash = next;
  if (path_file != slash) slash++;
  int path_len = slash - path_file;
  *path = intern_range(path_file, 0, path_len);
  char *dot = (ext == NULL) ? NULL : strchr(slash + 1, '.');
  int filename_len = (dot == NULL) ? strlen(path_file) - path_len : dot - slash;
  *file_name = intern_range(slash, 0, filename_len);
  if (dot != NULL) {
    *ext = intern(dot);
  } else if (ext != NULL) {
    *ext = NULL;
  }
}

char *combine_path_file(const char path[], const char file_name[],
                        const char ext[]) {
  int path_len = (ends_with(path, "/") || ends_with(path, "\\"))
                     ? strlen(path) - 1
                     : strlen(path);
  int filename_len = strlen(file_name);
  int ext_len = (NULL == ext) ? 0 : strlen(ext);
  int full_len = path_len + 1 + filename_len + ext_len;
  char *tmp = ALLOC_ARRAY2(char, full_len);
  memmove(tmp, path, path_len);
  tmp[path_len] = SLASH_CHAR;
  memmove(tmp + path_len + 1, file_name, filename_len);
  if (NULL != ext) {
    memmove(tmp + path_len + 1 + filename_len, ext, ext_len);
  }
  char *result = intern_range(tmp, 0, full_len);
  DEALLOC(tmp);
  return result;
}

ssize_t getline(char **lineptr, size_t *n, FILE *stream) {
  char *bufptr = NULL;
  char *p = bufptr;
  size_t size;
  int c;

  if (lineptr == NULL) {
    return -1;
  }
  if (stream == NULL) {
    return -1;
  }
  if (n == NULL) {
    return -1;
  }
  bufptr = *lineptr;
  size = *n;

  c = fgetc(stream);
  if (c == EOF) {
    return -1;
  }
  if (bufptr == NULL) {
    bufptr = ALLOC_ARRAY2(char, 128);
    if (bufptr == NULL) {
      return -1;
    }
    size = 128;
  }
  p = bufptr;
  while (c != EOF) {
    if ((p - bufptr) > (size - 1)) {
      size = size + 128;
      // Handle realloc
      int offset = p - bufptr;
      bufptr = REALLOC(bufptr, char, size);
      if (bufptr == NULL) {
        return -1;
      }
      p = bufptr + offset;
    }
    *(p++) = c;
    if (c == '\n') {
      break;
    }
    c = fgetc(stream);
  }

  *(p++) = '\0';
  *lineptr = bufptr;
  *n = size;

  return p - bufptr - 1;
}