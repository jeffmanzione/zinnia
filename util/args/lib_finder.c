// lib_finder.c
//
// Created on: May 22, 2020
//     Author: Jeff

#include "util/args/lib_finder.h"

#include <stdlib.h>

char *path_to_libs() { return getenv(PATH_ENV_VAR_NAME); }
