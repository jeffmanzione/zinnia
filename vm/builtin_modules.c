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
#include "entity/native/data.h"
#include "entity/native/dynamic.h"
#include "entity/native/error.h"
#include "entity/native/io.h"
#include "entity/native/math.h"
#include "entity/native/socket.h"
#include "entity/native/time.h"
#include "lib/lib.h"
#include "util/file.h"
#include "util/file/file_util.h"
#include "util/platform.h"
#include "util/string.h"

#define LIB_DIR "lib/"
#define LIB_EXT ".ja"

#define REGISTER_MODULE(mm, name, lib_location)                                \
  {                                                                            \
    if (NULL != lib_location) {                                                \
      mm_register_module(mm, find_file_by_name(lib_location, #name), NULL);    \
    } else {                                                                   \
      mm_register_module(mm, LIB_DIR #name LIB_EXT, LIB_##name);               \
    }                                                                          \
  }

#define REGISTER_MODULE_WITH_CALLBACK(mm, name, lib_location)                  \
  {                                                                            \
    if (NULL != lib_location) {                                                \
      mm_register_module_with_callback(mm,                                     \
                                       find_file_by_name(lib_location, #name), \
                                       NULL, name##_add_native);               \
    } else {                                                                   \
      mm_register_module_with_callback(mm, LIB_DIR #name LIB_EXT, LIB_##name,  \
                                       name##_add_native);                     \
    }                                                                          \
  }

void register_builtin(ModuleManager *mm, Heap *heap, const char *lib_location) {
  REGISTER_MODULE_WITH_CALLBACK(mm, builtin, lib_location);
  Module_builtin = modulemanager_lookup(mm, intern("builtin"));

  REGISTER_MODULE_WITH_CALLBACK(mm, io, lib_location);
  Module_io = modulemanager_lookup(mm, intern("io"));

  REGISTER_MODULE_WITH_CALLBACK(mm, error, lib_location);
  modulemanager_lookup(mm, intern("error"));

  REGISTER_MODULE_WITH_CALLBACK(mm, async, lib_location);
  REGISTER_MODULE(mm, struct, lib_location);
  REGISTER_MODULE_WITH_CALLBACK(mm, math, lib_location);
  REGISTER_MODULE_WITH_CALLBACK(mm, classes, lib_location);
  REGISTER_MODULE_WITH_CALLBACK(mm, socket, lib_location);
  REGISTER_MODULE(mm, net, lib_location);
  REGISTER_MODULE_WITH_CALLBACK(mm, dynamic, lib_location);
  REGISTER_MODULE_WITH_CALLBACK(mm, time, lib_location);
  REGISTER_MODULE(mm, builtin_ext, lib_location);
  REGISTER_MODULE(mm, io_ext, lib_location);
  REGISTER_MODULE(mm, memory, lib_location);
  REGISTER_MODULE(mm, test, lib_location);
  REGISTER_MODULE(mm, inject, lib_location);
  REGISTER_MODULE(mm, build, lib_location);
  REGISTER_MODULE_WITH_CALLBACK(mm, data, lib_location);
  REGISTER_MODULE(mm, json, lib_location);
}