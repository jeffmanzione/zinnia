// commandlines.c
//
// Created on: Jun 2, 2018
//     Author: Jeff

#include "zinnia/util/args/commandlines.h"

#include <stdbool.h>

#include "zinnia/util/args/commandline_arg.h"
#include "zinnia/util/args/lib_finder.h"
#include "zinnia/util/error.h"

void argconfig_compile(ArgConfig *const config) {
  ASSERT(config != NULL);
  argconfig_add(config, ArgKey__OUT_ASSEMBLY, "assembly", 'a', arg_bool(false));
  argconfig_add(config, ArgKey__OUT_BINARY, "bytecode", 'b', arg_bool(false));
  argconfig_add(config, Argkey__MINIMIZE, "minimize", 'm', arg_bool(false));
  argconfig_add(config, ArgKey__BIN_OUT_DIR, "binary_out_dir", '\0',
                arg_string("./"));
  argconfig_add(config, ArgKey__ASSEMBLY_OUT_DIR, "assembly_out_dir", '\0',
                arg_string("./"));
  argconfig_add(config, ArgKey__OPTIMIZE, "optimize", 'o', arg_bool(true));
}

void argconfig_run(ArgConfig *const config) {
  ASSERT(config != NULL);
  argconfig_add(config, ArgKey__LIB_LOCATION, "zinnia/lib_location", '\0',
                arg_string(path_to_libs()));
  argconfig_add(config, ArgKey__MAX_PROCESS_OBJECT_COUNT,
                "zinnia/heap_object_limit", '\0', arg_int(4096 * 8));
  argconfig_add(config, ArgKey__ASYNC, "async", '\0', arg_bool(true));
}

void argconfig_package(ArgConfig *const config) {
  ASSERT(config != NULL);
  argconfig_run(config);
  argconfig_set_allow_compiler_args_and_sources(config, false);
}