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
  argconfig_add(config, ArgKey__OUT_MACHINE, "m", arg_bool(false));
  argconfig_add(config, ArgKey__OUT_BINARY, "b", arg_bool(false));
  argconfig_add(config, ArgKey__OPTIMIZE, "opt", arg_bool(true));
  argconfig_add(config, ArgKey__OUT_UNOPTIMIZED, "ou", arg_bool(false));
  argconfig_add(config, ArgKey__BIN_OUT_DIR, "bout", arg_string("./"));
  argconfig_add(config, ArgKey__MACHINE_OUT_DIR, "mout", arg_string("./"));
  argconfig_add(config, ArgKey__UNOPTIMIZED_OUT_DIR, "uoout", arg_string("./"));
}

void argconfig_run(ArgConfig *const config) {
  ASSERT(NOT_NULL(config));
  argconfig_add(config, ArgKey__EXECUTE, "ex", arg_bool(true));
  argconfig_add(config, ArgKey__INTERPRETER, "i", arg_bool(false));
  argconfig_add(config, ArgKey__BUILTIN_DIR, "builtin_dir",
                arg_string(path_to_libs()));
  argconfig_add(config, ArgKey__BUILTIN_FILES, "builtin_files",
                arg_stringlist("error,io,struct"));
}