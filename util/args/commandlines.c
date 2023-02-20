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
  argconfig_add(config, ArgKey__OUT_ASSEMBLY, "assembly", 'a', arg_bool(false));
  argconfig_add(config, ArgKey__OUT_BINARY, "bytecode", 'b', arg_bool(false));
  argconfig_add(config, ArgKey__BIN_OUT_DIR, "binary_out_dir", '\0',
                arg_string("./"));
  argconfig_add(config, ArgKey__ASSEMBLY_OUT_DIR, "assembly_out_dir", '\0',
                arg_string("./"));
  argconfig_add(config, ArgKey__OPTIMIZE, "optimize", 'o', arg_bool(true));
}

void argconfig_run(ArgConfig *const config) {
  ASSERT(NOT_NULL(config));
  argconfig_add(config, ArgKey__LIB_LOCATION, "lib_location", '\0',
                arg_string(path_to_libs()));
  argconfig_add(config, ArgKey__MAX_PROCESS_OBJECT_COUNT, "heap_object_limit",
                '\0', arg_int(4096 * 8));
  argconfig_add(config, ArgKey__ASYNC, "async", '\0', arg_bool(true));
  argconfig_add(config, ArgKey__INTERPRETER, "interpreter", 'i',
                arg_bool(false));
}