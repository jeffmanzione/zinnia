// builtin_modules.c
//
// Created on: Jan 1, 2021
//     Author: Jeff Manzione

#include "vm/builtin_modules.h"

#include <dirent.h>
#include <stdio.h>

#include "alloc/arena/intern.h"
#include "entity/class/classes.h"
#include "entity/module/module.h"
#include "entity/module/modules.h"
#include "entity/native/async.h"
#include "entity/native/builtin.h"
#include "entity/native/classes.h"
#include "entity/native/dynamic.h"
#include "entity/native/error.h"
#include "entity/native/io.h"
#include "entity/native/math.h"
#include "entity/native/process.h"
#include "entity/native/socket.h"
#include "util/file.h"
#include "util/file/file_util.h"
#include "util/string.h"

void register_builtin(ModuleManager *mm, Heap *heap, const char *lib_location) {
  mm_register_module_with_callback(
      mm, find_file_by_name(lib_location, "builtin"), builtin_add_native);
  Module_builtin = modulemanager_lookup(mm, intern("builtin"));

  mm_register_module_with_callback(mm, find_file_by_name(lib_location, "io"),
                                   io_add_native);
  Module_io = modulemanager_lookup(mm, intern("io"));

  mm_register_module_with_callback(mm, find_file_by_name(lib_location, "error"),
                                   error_add_native);
  modulemanager_lookup(mm, intern("error"));

  mm_register_module_with_callback(mm, find_file_by_name(lib_location, "async"),
                                   async_add_native);
  modulemanager_lookup(mm, intern("async"));

  mm_register_module(mm, find_file_by_name(lib_location, "struct"));
  mm_register_module_with_callback(mm, find_file_by_name(lib_location, "math"),
                                   math_add_native);
  mm_register_module_with_callback(
      mm, find_file_by_name(lib_location, "classes"), classes_add_native);
  mm_register_module_with_callback(
      mm, find_file_by_name(lib_location, "process"), process_add_native);
  mm_register_module_with_callback(
      mm, find_file_by_name(lib_location, "socket"), socket_add_native);
  mm_register_module(mm, find_file_by_name(lib_location, "net"));
  mm_register_module_with_callback(
      mm, find_file_by_name(lib_location, "dynamic"), dynamic_add_native);

  DIR *lib = opendir(lib_location);
  struct dirent *dir;
  while ((dir = readdir(lib)) != NULL) {
    const char *fn = combine_path_file(lib_location, dir->d_name, NULL);
    DEBUGF("'%s' '%s'", dir->d_name, fn);
    if (ends_with(dir->d_name, ".jv") || ends_with(dir->d_name, ".ja") ||
        ends_with(dir->d_name, ".jb")) {
      mm_register_module(mm, fn);
    }
  }
}