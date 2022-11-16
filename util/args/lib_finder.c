// lib_finder.c
//
// Created on: May 22, 2020
//     Author: Jeff

#include "util/args/lib_finder.h"

#include <stdlib.h>

#define DEFAULT_PATH_TO_LIB "./lib"

char *path_to_libs() {
  char *path_var = getenv(PATH_ENV_VAR_NAME);
  if (NULL == path_var) {
    path_var = DEFAULT_PATH_TO_LIB;
  }
  return path_var;
}
