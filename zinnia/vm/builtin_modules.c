// builtin_modules.c
//
// Created on: Jan 1, 2021
//     Author: Jeff Manzione

#include "zinnia/vm/builtin_modules.h"

#include <stdio.h>

#include "file-utils/file_utils.h"
#include "file-utils/string_utils.h"
#include "zinnia/entity/class/classes_def.h"
#include "zinnia/entity/module/module.h"
#include "zinnia/entity/module/modules.h"
#include "zinnia/entity/native/async.h"
#include "zinnia/entity/native/builtin.h"
#include "zinnia/entity/native/classes.h"
#include "zinnia/entity/native/data.h"
#include "zinnia/entity/native/dynamic.h"
#include "zinnia/entity/native/error.h"
#include "zinnia/entity/native/io.h"
#include "zinnia/entity/native/math.h"
#include "zinnia/entity/native/socket.h"
#include "zinnia/entity/native/time.h"
#include "zinnia/lib/lib.h"
#include "zinnia/util/file.h"
#include "zinnia/util/platform.h"
#include "zinnia/vm/intern.h"

#define LIB_DIR "zinnia/lib/"
#define LIB_EXT ".zna"

#define REGISTER_MODULE(mm, name, lib_location)                             \
  {                                                                         \
    if (NULL != lib_location) {                                             \
      mm_register_module(mm, find_file_by_name(lib_location, #name),        \
                         find_file_by_name(lib_location, #name), NULL, -1); \
    } else {                                                                \
      mm_register_module(mm, LIB_DIR #name LIB_EXT, LIB_DIR #name LIB_EXT,  \
                         LIB_##name,                                        \
                         sizeof(LIB_##name) / sizeof(LIB_##name[0]));       \
    }                                                                       \
  }

#define REGISTER_MODULE_WITH_CALLBACK(mm, name, lib_location)                  \
  {                                                                            \
    if (NULL != lib_location) {                                                \
      mm_register_module_with_callback(mm,                                     \
                                       find_file_by_name(lib_location, #name), \
                                       find_file_by_name(lib_location, #name), \
                                       NULL, -1, name##_add_native);           \
    } else {                                                                   \
      mm_register_module_with_callback(                                        \
          mm, LIB_DIR #name LIB_EXT, LIB_DIR #name LIB_EXT, LIB_##name,        \
          sizeof(LIB_##name) / sizeof(LIB_##name[0]), name##_add_native);      \
    }                                                                          \
  }

#define REGISTER_MODULE_WITH_CALLBACK2(mm, name, lib_location)            \
  {                                                                       \
    if (NULL != lib_location) {                                           \
      mm_register_module_with_callback2(                                  \
          mm, find_file_by_name(lib_location, #name),                     \
          find_file_by_name(lib_location, #name), NULL, -1,               \
          name##_add_native);                                             \
    } else {                                                              \
      mm_register_module_with_callback2(                                  \
          mm, LIB_DIR #name LIB_EXT, LIB_DIR #name LIB_EXT, LIB_##name,   \
          sizeof(LIB_##name) / sizeof(LIB_##name[0]), name##_add_native); \
    }                                                                     \
  }

void register_builtin(ModuleManager *mm, Heap *heap, const char *lib_location) {
  REGISTER_MODULE_WITH_CALLBACK(mm, builtin, lib_location);
  Module_builtin = modulemanager_lookup(mm, global_intern("builtin"));

  REGISTER_MODULE_WITH_CALLBACK(mm, io, lib_location);
  Module_io = modulemanager_lookup(mm, global_intern("io"));

  REGISTER_MODULE_WITH_CALLBACK(mm, error, lib_location);
  modulemanager_lookup(mm, global_intern("error"));

  REGISTER_MODULE_WITH_CALLBACK(mm, async, lib_location);
  REGISTER_MODULE(mm, struct, lib_location);
  REGISTER_MODULE_WITH_CALLBACK(mm, math, lib_location);
  REGISTER_MODULE_WITH_CALLBACK(mm, classes, lib_location);
  REGISTER_MODULE_WITH_CALLBACK(mm, socket, lib_location);
  REGISTER_MODULE(mm, net, lib_location);
  REGISTER_MODULE_WITH_CALLBACK(mm, dynamic, lib_location);
  REGISTER_MODULE_WITH_CALLBACK2(mm, time, lib_location);
  REGISTER_MODULE(mm, builtin_ext, lib_location);
  REGISTER_MODULE(mm, io_ext, lib_location);
  REGISTER_MODULE(mm, memory, lib_location);
  REGISTER_MODULE(mm, test, lib_location);
  REGISTER_MODULE(mm, inject, lib_location);
  REGISTER_MODULE(mm, build, lib_location);
  REGISTER_MODULE_WITH_CALLBACK(mm, data, lib_location);
  REGISTER_MODULE(mm, json, lib_location);
}