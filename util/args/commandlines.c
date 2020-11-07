// commandlines.c
//
// Created on: Jun 2, 2018
//     Author: Jeff

#include "util/args/commandlines.h"

#include <stdbool.h>

#include "debug/debug.h"
#include "util/args/commandline_arg.h"
#include "util/args/lib_finder.h"

void argconfig_compile(ArgConfig *const config) {
  ASSERT(NOT_NULL(config));
  argconfig_add(config, ArgKey__OUT_ASSEMBLY, "a", arg_bool(false));
  argconfig_add(config, ArgKey__OUT_BINARY, "b", arg_bool(false));
  argconfig_add(config, ArgKey__BIN_OUT_DIR, "bout", arg_string("./"));
  argconfig_add(config, ArgKey__ASSEMBLY_OUT_DIR, "aout", arg_string("./"));
  argconfig_add(config, ArgKey__OPTIMIZE, "o", arg_bool(true));
}

void argconfig_run(ArgConfig *const config) { ASSERT(NOT_NULL(config)); }