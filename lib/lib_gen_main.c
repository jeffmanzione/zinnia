// lib_gen_main.c
//
// Created on: Nov 15, 2022
//     Author: Jeff Manzione

#include <stdio.h>
#include <stdlib.h>

#include "alloc/alloc.h"
#include "util/file/file_util.h"
#include "util/string_util.h"

#define STRING_BLOCK_LEN 400

int main(int argc, const char *args[]) {
  const char *out_file_name = args[1];
  FILE *out = FILE_FN(out_file_name, "w");
  if (NULL == out) {
    fprintf(stderr, "Could not open file: \"%s\".", out_file_name);
    return EXIT_FAILURE;
  }
  for (int i = 2; i < argc; ++i) {
    const char *input_file_name = args[i];
    char *dir_path, *file_base, *ext;
    split_path_file(input_file_name, &dir_path, &file_base, &ext);
    FILE *input_file = FILE_FN(input_file_name, "rb");
    if (NULL == input_file) {
      fprintf(stderr, "Could not open file: \"%s\".", input_file_name);
      return EXIT_FAILURE;
    }
    char *input;
    getall(input_file, &input);
    const char *escaped_input = escape(input);
    fprintf(out, "const char LIB_%s[] =", file_base);
    int start = 0, end = STRING_BLOCK_LEN;
    for (; end <= strlen(escaped_input); end += STRING_BLOCK_LEN) {
      while (escaped_input[end] != ' ' && end < strlen(escaped_input)) {
        ++end;
      }
      int len = min(end - start, strlen(escaped_input + start));
      fprintf(out, "\n  \"%.*s\"", len, escaped_input + start);
      start = end;
    }
    if (start < strlen(escaped_input)) {
      fprintf(out, "\n  \"%.*s\"", (int)strlen(escaped_input + start),
              escaped_input + start);
    }
    fprintf(out, ";\n\n");

    DEALLOC(dir_path);
    DEALLOC(file_base);
    DEALLOC(ext);
    DEALLOC(input);
    DEALLOC(escaped_input);

    fclose(input_file);
  }
  fclose(out);
  return EXIT_SUCCESS;
}