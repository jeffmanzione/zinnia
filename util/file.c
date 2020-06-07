// file.c
//
// Created on: Jun 6, 2020
//     Author: Jeff Manzione

#include "util/file.h"

#include <stdio.h>

#include "debug/debug.h"

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