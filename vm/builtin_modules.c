// builtin_modules.c
//
// Created on: Jan 1, 2021
//     Author: Jeff Manzione

#include "vm/builtin_modules.h"

#include <stdio.h>

#include "alloc/arena/intern.h"
#include "entity/class/classes_def.h"
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
#include "entity/native/time.h"
#include "lib/lib.h"
#include "util/file.h"
#include "util/file/file_util.h"
#include "util/platform.h"
#include "util/string.h"

#define LIB_DIR "lib/"
#define LIB_EXT ".ja"

void register_builtin(ModuleManager *mm, Heap *heap, const char *lib_location) {
  mm_register_module_with_callback(mm, LIB_DIR "builtin" LIB_EXT, LIB_builtin,
                                   builtin_add_native);
  Module_builtin = modulemanager_lookup(mm, intern("builtin"));

  mm_register_module_with_callback(mm, LIB_DIR "io" LIB_EXT, LIB_io,
                                   io_add_native);
  Module_io = modulemanager_lookup(mm, intern("io"));

  mm_register_module_with_callback(mm, LIB_DIR "error" LIB_EXT, LIB_error,
                                   error_add_native);
  modulemanager_lookup(mm, intern("error"));

  mm_register_module_with_callback(mm, LIB_DIR "async" LIB_EXT, LIB_async,
                                   async_add_native);
  modulemanager_lookup(mm, intern("async"));

  mm_register_module(mm, LIB_DIR "struct" LIB_EXT, LIB_struct);
  mm_register_module_with_callback(mm, LIB_DIR "math" LIB_EXT, LIB_math,
                                   math_add_native);
  mm_register_module_with_callback(mm, LIB_DIR "classes" LIB_EXT, LIB_classes,
                                   classes_add_native);
  mm_register_module_with_callback(mm, LIB_DIR "process" LIB_EXT, LIB_process,
                                   process_add_native);
  mm_register_module_with_callback(mm, LIB_DIR "socket" LIB_EXT, LIB_socket,
                                   socket_add_native);
  mm_register_module(mm, LIB_DIR "net" LIB_EXT, LIB_net);
  mm_register_module_with_callback(mm, LIB_DIR "dynamic" LIB_EXT, LIB_dynamic,
                                   dynamic_add_native);
  mm_register_module_with_callback(mm, LIB_DIR "time" LIB_EXT, LIB_time,
                                   time_add_native);

#ifdef OS_WINDOWS
  char *lib_buf = ALLOC_ARRAY2(char, strlen(lib_location) + 3);
  memmove(lib_buf, lib_location, strlen(lib_location));
  memmove(lib_buf + strlen(lib_location), "/*", 3);
  DirIter *di = directory_iter(lib_buf);
#else
  DirIter *di = directory_iter(lib_location);
#endif
  const char *file_name;
  while ((file_name = diriter_next_file(di)) != NULL) {
    const char *fn = combine_path_file(lib_location, file_name, NULL);
    if (ends_with(file_name, ".jv") || ends_with(file_name, ".ja") ||
        ends_with(file_name, ".jb")) {
      mm_register_module(mm, fn, NULL);
    }
    DEALLOC(fn);
  }
  diriter_close(di);
#ifdef OS_WINDOWS
  DEALLOC(lib_buf);
#endif
}