// lib_finder.c
//
// Created on: May 22, 2020
//     Author: Jeff

#include "zinnia/util/args/lib_finder.h"

#include <stdlib.h>

#define PATH_ENV_VAR_NAME "JV_LIB_PATH"

char *path_to_libs() { return getenv(PATH_ENV_VAR_NAME); }
