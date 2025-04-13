// lib_gen_main.c
//
// Created on: Nov 15, 2022
//     Author: Jeff Manzione

#include <stdio.h>
#include <stdlib.h>

#include "alloc/alloc.h"
#include "util/file/file_util.h"
#include "util/string_util.h"

// This *should* keep the individual concatenated liberals < 509 characters
// which is the max length of string literal in ANSI.
#define LOOSE_LITERAL_PRECAT_UPPER_BOUND 100
#define LOOSE_LITERAL_UPPER_BOUND 480

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

int main(int argc, const char *args[]) {
  const char *out_file_name = args[1];
  FILE *out = FILE_FN(out_file_name, "w");
  if (NULL == out) {
    fprintf(stderr, "Could not open file: \"%s\".", out_file_name);
    return EXIT_FAILURE;
  }

  fprintf(out, "#ifndef ZINNIA_LIB_LIB_H_\n#define ZINNIA_LIB_LIB_H_\n\n");
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
    const escaped_input_len = strlen(escaped_input);
    fprintf(out, "const char *LIB_%s[] = {", file_base);
    int start = 0, end = LOOSE_LITERAL_PRECAT_UPPER_BOUND;
    int current_literal_len = 0;
    for (; end <= escaped_input_len; end += LOOSE_LITERAL_PRECAT_UPPER_BOUND) {
      while (escaped_input[end] != ' ' && end < escaped_input_len) {
        ++end;
      }
      int len = min(end - start, escaped_input_len - start);
      current_literal_len += len;
      fprintf(out, "\n  \"%.*s\"", len, escaped_input + start);
      start = end;

      if (current_literal_len > LOOSE_LITERAL_UPPER_BOUND) {
        fprintf(out, ",");
        current_literal_len = 0;
      }
    }
    if (start < escaped_input_len) {
      // Means that a comma was just added.
      fprintf(out, "\n  \"%.*s\"", escaped_input_len - start,
              escaped_input + start);
    }
    fprintf(out, "};\n\n");

    RELEASE(dir_path);
    RELEASE(file_base);
    RELEASE(ext);
    RELEASE(input);
    RELEASE(escaped_input);

    // fclose(input_file);
  }

  fprintf(out, "#endif /* ZINNIA_LIB_LIB_H_ */\n");

  fclose(out);
  return EXIT_SUCCESS;
}