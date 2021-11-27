// dynamic.c
//
// Created on: Nov 12, 2021
//     Author: Jeff Manzione

#include "entity/native/dynamic.h"

#include <dlfcn.h>

#include "alloc/arena/intern.h"
#include "entity/class/classes.h"
#include "entity/entity.h"
#include "entity/native/error.h"
#include "entity/native/native.h"
#include "entity/string/string.h"
#include "entity/tuple/tuple.h"
#include "vm/module_manager.h"
#include "vm/process/processes.h"
#include "vm/vm.h"

Entity _open_c_lib(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (!IS_TUPLE(args)) {
    return raise_error(task, ctx,
                       "Invalid arguments for __open_c_lib: Input is the "
                       "wrong type. Expected tuple(3).");
  }
  const Tuple *t = (Tuple *)args->obj->_internal_obj;
  if (3 != tuple_size(t)) {
    return raise_error(task, ctx,
                       "Invalid number of arguments for "
                       "__open_c_lib: Expected 3 arguments but got %d.",
                       tuple_size(t));
  }
  const Entity *arg0 = tuple_get(t, 0);
  const Entity *arg1 = tuple_get(t, 1);
  const Entity *arg2 = tuple_get(t, 2);
  if (!IS_CLASS(arg0, Class_String)) {
    return raise_error(task, ctx,
                       "Invalid argument(0) for "
                       "__open_c_lib: Expected type String.");
  }
  if (!IS_CLASS(arg1, Class_String)) {
    return raise_error(task, ctx,
                       "Invalid argument(1) for "
                       "__open_c_lib: Expected type String.");
  }
  if (!IS_CLASS(arg2, Class_String)) {
    return raise_error(task, ctx,
                       "Invalid argument(2) for "
                       "__open_c_lib: Expected type String.");
  }
  String *str_arg0 = (String *)arg0->obj->_internal_obj;
  String *str_arg1 = (String *)arg1->obj->_internal_obj;
  String *str_arg2 = (String *)arg2->obj->_internal_obj;
  char *file_name = strndup(str_arg0->table, String_size(str_arg0));
  char *module_name = strndup(str_arg1->table, String_size(str_arg1));
  char *init_fn_name = strndup(str_arg2->table, String_size(str_arg2));

  void *dl_handle = dlopen(file_name, RTLD_LAZY);
  if (NULL == dl_handle) {
    return raise_error(task, ctx, "Invalid file_name for library: %s",
                       dlerror());
  }
  NativeCallback init_fn = (NativeCallback)dlsym(dl_handle, init_fn_name);
  if (NULL == init_fn) {
    return raise_error(task, ctx, "init_fn_name not found in library: %s",
                       dlerror());
  }
  ModuleManager *mm = vm_module_manager(task->parent_process->vm);
  mm_register_dynamic_module(mm, module_name, init_fn);
  // Forces module to be loaded eagerly.
  Module *module = modulemanager_lookup(mm, module_name);
  return entity_object(module->_reflection);
}

void dynamic_add_native(ModuleManager *mm, Module *dynamic) {
  native_function(dynamic, intern("__open_c_lib"), _open_c_lib);
}