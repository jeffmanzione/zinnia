// lib_gen_main.c
//
// Created on: Nov 15, 2022
//     Author: Jeff Manzione

#include <stdio.h>
#include <stdlib.h>

#include "alloc/alloc.h"
#include "util/codegen.h"
#include "util/file/file_util.h"

#define MAX_VAR_NAME_LEN 127

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

    char var_name[MAX_VAR_NAME_LEN + 1];
    var_name[0] = 0x0;
    sprintf(var_name, "LIB_%s", file_base);

    char *input;
    getall(input_file, &input);

    print_string_as_var(var_name, input, out);

    RELEASE(dir_path);
    RELEASE(file_base);
    RELEASE(ext);
    RELEASE(input);

    // fclose(input_file);
  }

  fprintf(out, "#endif /* ZINNIA_LIB_LIB_H_ */\n");

  fclose(out);
  return EXIT_SUCCESS;
}