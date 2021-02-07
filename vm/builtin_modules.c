// builtin_modules.c
//
// Created on: Jan 1, 2021
//     Author: Jeff Manzione

#include "vm/builtin_modules.h"

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

void read_builtin(ModuleManager *mm, Heap *heap) {
  // builtin.jv
  Module_builtin = mm_read_helper(mm, "lib/builtin.jv");
  builtin_classes(heap, Module_builtin);
  builtin_add_native(Module_builtin);
  add_reflection_to_module(mm, Module_builtin);
  heap_make_root(heap, Module_builtin->_reflection);
  // io.jv
  Module_io = mm_read_helper(mm, "lib/io.jv");
  io_add_native(Module_io);
  add_reflection_to_module(mm, Module_io);
  heap_make_root(heap, Module_io->_reflection);
  // error.jv
  Module_error = mm_read_helper(mm, "lib/error.jv");
  error_add_native(Module_error);
  add_reflection_to_module(mm, Module_error);
  heap_make_root(heap, Module_error->_reflection);
  // async.jv
  Module_async = mm_read_helper(mm, "lib/async.jv");
  async_add_native(Module_async);
  add_reflection_to_module(mm, Module_async);
  heap_make_root(heap, Module_async->_reflection);
  // math.jv
  Module_math = mm_read_helper(mm, "lib/math.jv");
  math_add_native(Module_math);
  add_reflection_to_module(mm, Module_math);
  heap_make_root(heap, Module_math->_reflection);
  // struct.jv
  Module_struct = mm_read_helper(mm, "lib/struct.jv");
  add_reflection_to_module(mm, Module_struct);
  heap_make_root(heap, Module_struct->_reflection);
  // classes.jv
  Module_classes = mm_read_helper(mm, "lib/classes.jv");
  classes_add_native(Module_classes);
  add_reflection_to_module(mm, Module_classes);
  heap_make_root(heap, Module_classes->_reflection);
  // process.jv
  Module_process = mm_read_helper(mm, "lib/process.jv");
  process_add_native(Module_process);
  add_reflection_to_module(mm, Module_process);
  heap_make_root(heap, Module_process->_reflection);
}