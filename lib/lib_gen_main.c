// lib_gen_main.c
//
// Created on: Nov 15, 2022
//     Author: Jeff Manzione

#include <stdio.h>
#include <stdlib.h>

#include "alloc/alloc.h"
#include "util/file/file_util.h"
#include "util/string_util.h"

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
    char *input;
    getall(input_file, &input);
    const char *escaped_input = escape(input);
    fprintf(out, "const char LIB_%s[] = \"%s\";\n\n", file_base, escaped_input);

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