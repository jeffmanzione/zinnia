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
#include "util/file.h"
#include "util/string.h"

const char *BUILTIN_LIBRARIES = {"builtin", "io",     "error",   "async",
                                 "math",    "struct", "classes", "process",
                                 "socket",  "net"};

void read_builtin(ModuleManager *mm, Heap *heap, const char *lib_location) {
  // builtin.jv
  {
    const char *fn = find_file_by_name(lib_location, "builtin");
    Module_builtin = mm_read_helper(mm, fn);
    builtin_classes(heap, Module_builtin);
    builtin_add_native(Module_builtin);
    add_reflection_to_module(mm, Module_builtin);
    heap_make_root(heap, Module_builtin->_reflection);
  }
  // io.jv
  {
    const char *fn = find_file_by_name(lib_location, "io");
    Module_io = mm_read_helper(mm, fn);
    io_add_native(Module_io);
    add_reflection_to_module(mm, Module_io);
    heap_make_root(heap, Module_io->_reflection);
  }
  // error.jv
  {
    const char *fn = find_file_by_name(lib_location, "error");
    Module_error = mm_read_helper(mm, fn);
    error_add_native(Module_error);
    add_reflection_to_module(mm, Module_error);
    heap_make_root(heap, Module_error->_reflection);
  }
  // async.jv
  {
    const char *fn = find_file_by_name(lib_location, "async");
    Module_async = mm_read_helper(mm, fn);
    async_add_native(Module_async);
    add_reflection_to_module(mm, Module_async);
    heap_make_root(heap, Module_async->_reflection);
  }
  // math.jv
  {
    const char *fn = find_file_by_name(lib_location, "math");
    Module_math = mm_read_helper(mm, fn);
    math_add_native(Module_math);
    add_reflection_to_module(mm, Module_math);
    heap_make_root(heap, Module_math->_reflection);
  }
  // struct.jv
  {
    const char *fn = find_file_by_name(lib_location, "struct");
    Module_struct = mm_read_helper(mm, fn);
    add_reflection_to_module(mm, Module_struct);
    heap_make_root(heap, Module_struct->_reflection);
  }
  // classes.jv
  {
    const char *fn = find_file_by_name(lib_location, "classes");
    Module_classes = mm_read_helper(mm, fn);
    classes_add_native(Module_classes);
    add_reflection_to_module(mm, Module_classes);
    heap_make_root(heap, Module_classes->_reflection);
  }
  // process.jv
  {
    const char *fn = find_file_by_name(lib_location, "process");
    Module_process = mm_read_helper(mm, fn);
    process_add_native(Module_process);
    add_reflection_to_module(mm, Module_process);
    heap_make_root(heap, Module_process->_reflection);
  }
  // socket.jv
  {
    const char *fn = find_file_by_name(lib_location, "socket");
    Module_socket = mm_read_helper(mm, fn);
    socket_add_native(Module_socket);
    add_reflection_to_module(mm, Module_socket);
    heap_make_root(heap, Module_socket->_reflection);
  }
  // net.jv
  {
    const char *fn = find_file_by_name(lib_location, "net");
    Module_net = mm_read_helper(mm, fn);
    add_reflection_to_module(mm, Module_net);
    heap_make_root(heap, Module_net->_reflection);
  }

  DIR *lib = opendir(lib_location);
  struct dirent *dir;
  while ((dir = readdir(lib)) != NULL) {
    const char *fn = find_file_by_name(lib_location, dir->d_name);
    if (ends_with(fn, ".jv") || ends_with(fn, ".ja") || ends_with(fn, ".jb")) {
      modulemanager_read(mm, fn);
    }
  }
}

void maybe_add_native(ModuleInfo *mi) {
  if (ends_with(module_info_file_name(mi), "builtin.jl")) {
  }
}