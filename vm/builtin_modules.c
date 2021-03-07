// builtin_modules.c
//
// Created on: Jan 1, 2021
//     Author: Jeff Manzione

#include "vm/builtin_modules.h"

#include <dirent.h>
#include <stdio.h>

#include "entity/class/classes.h"
#include "entity/module/module.h"
#include "entity/module/modules.h"
#include "entity/native/async.h"
#include "entity/native/builtin.h"
#include "entity/native/classes.h"
#include "entity/native/error.h"
#include "entity/native/io.h"
#include "entity/native/math.h"
#include "entity/native/process.h"
#include "entity/native/socket.h"
#include "util/string.h"

#define LIB_FILE_EXT ".jv"

void read_builtin(ModuleManager *mm, Heap *heap, const char *lib_location) {
  // builtin.jv
  {
    const char *fn = combine_path_file(lib_location, "builtin", LIB_FILE_EXT);
    Module_builtin = mm_read_helper(mm, fn);
    builtin_classes(heap, Module_builtin);
    builtin_add_native(Module_builtin);
    add_reflection_to_module(mm, Module_builtin);
    heap_make_root(heap, Module_builtin->_reflection);
    DEALLOC(fn);
  }
  // io.jv
  {
    const char *fn = combine_path_file(lib_location, "io", LIB_FILE_EXT);
    Module_io = mm_read_helper(mm, fn);
    io_add_native(Module_io);
    add_reflection_to_module(mm, Module_io);
    heap_make_root(heap, Module_io->_reflection);
    DEALLOC(fn);
  }
  // error.jv
  {
    const char *fn = combine_path_file(lib_location, "error", LIB_FILE_EXT);
    Module_error = mm_read_helper(mm, fn);
    error_add_native(Module_error);
    add_reflection_to_module(mm, Module_error);
    heap_make_root(heap, Module_error->_reflection);
    DEALLOC(fn);
  }
  // async.jv
  {
    const char *fn = combine_path_file(lib_location, "async", LIB_FILE_EXT);
    Module_async = mm_read_helper(mm, fn);
    async_add_native(Module_async);
    add_reflection_to_module(mm, Module_async);
    heap_make_root(heap, Module_async->_reflection);
    DEALLOC(fn);
  }
  // math.jv
  {
    const char *fn = combine_path_file(lib_location, "math", LIB_FILE_EXT);
    Module_math = mm_read_helper(mm, fn);
    math_add_native(Module_math);
    add_reflection_to_module(mm, Module_math);
    heap_make_root(heap, Module_math->_reflection);
    DEALLOC(fn);
  }
  // struct.jv
  {
    const char *fn = combine_path_file(lib_location, "struct", LIB_FILE_EXT);
    Module_struct = mm_read_helper(mm, fn);
    add_reflection_to_module(mm, Module_struct);
    heap_make_root(heap, Module_struct->_reflection);
    DEALLOC(fn);
  }
  // classes.jv
  {
    const char *fn = combine_path_file(lib_location, "classes", LIB_FILE_EXT);
    Module_classes = mm_read_helper(mm, fn);
    classes_add_native(Module_classes);
    add_reflection_to_module(mm, Module_classes);
    heap_make_root(heap, Module_classes->_reflection);
    DEALLOC(fn);
  }
  // process.jv
  {
    const char *fn = combine_path_file(lib_location, "process", LIB_FILE_EXT);
    Module_process = mm_read_helper(mm, fn);
    process_add_native(Module_process);
    add_reflection_to_module(mm, Module_process);
    heap_make_root(heap, Module_process->_reflection);
    DEALLOC(fn);
  }
  // socket.jv
  {
    const char *fn = combine_path_file(lib_location, "socket", LIB_FILE_EXT);
    Module_socket = mm_read_helper(mm, fn);
    socket_add_native(Module_socket);
    add_reflection_to_module(mm, Module_socket);
    heap_make_root(heap, Module_socket->_reflection);
    DEALLOC(fn);
  }
  // net.jv
  {
    const char *fn = combine_path_file(lib_location, "net", LIB_FILE_EXT);
    Module_net = mm_read_helper(mm, fn);
    add_reflection_to_module(mm, Module_net);
    heap_make_root(heap, Module_net->_reflection);
    DEALLOC(fn);
  }

  DIR *lib = opendir(lib_location);
  struct dirent *dir;
  while ((dir = readdir(lib)) != NULL) {
    const char *fn = combine_path_file(lib_location, dir->d_name, "");
    if (ends_with(fn, ".jv") || ends_with(fn, ".ja") || ends_with(fn, ".jb")) {
      modulemanager_read(mm, fn);
    }
    DEALLOC(fn);
  }
}