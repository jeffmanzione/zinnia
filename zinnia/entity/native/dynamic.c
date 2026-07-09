// dynamic.c
//
// Created on: Nov 12, 2021
//     Author: Jeff Manzione

#include "zinnia/entity/native/dynamic.h"

#include "zinnia/util/dll.h"

Entity open_c_lib_(Task *task, Context *ctx, Object *obj, Entity *args) {
  EXTRACT_TUPLE_ARGS(t, args, 3, task, ctx);

  const Entity *arg0 = tuple_get(t, 0);
  const Entity *arg1 = tuple_get(t, 1);
  const Entity *arg2 = tuple_get(t, 2);
  if (!IS_STRING(arg0)) {
    return raise_error(task, ctx,
                       "Invalid argument(0) for "
                       "__open_c_lib: Expected type String.");
  }
  if (!IS_STRING(arg1)) {
    return raise_error(task, ctx,
                       "Invalid argument(1) for "
                       "__open_c_lib: Expected type String.");
  }
  if (!IS_NONE(arg2) && !IS_STRING(arg2)) {
    return raise_error(task, ctx,
                       "Invalid argument(2) for "
                       "__open_c_lib: Expected None or String.");
  }
  char *file_name = entity_string_copy(arg0);
  char *module_name = entity_string_copy(arg1);

  const bool has_init = !IS_NONE(arg2);
  char *init_fn_name = has_init ? entity_string_copy(arg2) : NULL;

  DlHandle dl = NULL;
  char error_buf[255];
  if (!open_dl(file_name, &dl, error_buf)) {
    RELEASE(file_name);
    RELEASE(module_name);
    if (has_init) RELEASE(init_fn_name);
    return raise_error(task, ctx, error_buf);
  }

  NativeModuleBuilderInitFn init_fn = NULL;
  if (has_init &&
      !open_dl_sym(dl, init_fn_name, (DlFnHandle *)&init_fn, error_buf)) {
    RELEASE(file_name);
    RELEASE(module_name);
    if (has_init) RELEASE(init_fn_name);
    return raise_error(task, ctx, error_buf);
  }

  ModuleManager *mm = vm_module_manager(task->parent_process->vm);
  mm_register_dynamic_module(mm, module_name, dl, init_fn);
  // Forces module to be loaded eagerly.
  Module *module = modulemanager_lookup(mm, module_name);
  RELEASE(file_name);
  RELEASE(module_name);
  if (has_init) RELEASE(init_fn_name);
  return entity_object(module->_reflection);
}

void dynamic_add_native(ModuleManager *mm, Module *dynamic) {
  native_function(dynamic, global_intern("__open_c_lib"), open_c_lib_);
}