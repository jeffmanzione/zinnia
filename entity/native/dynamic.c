// dynamic.c
//
// Created on: Nov 12, 2021
//     Author: Jeff Manzione

#include "entity/native/dynamic.h"

#include "util/platform.h"

#ifdef OS_WINDOWS
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "alloc/arena/intern.h"
#include "entity/class/classes_def.h"
#include "entity/entity.h"
#include "entity/native/error.h"
#include "entity/native/native.h"
#include "entity/native/native_helpers.h"
#include "entity/string/string.h"
#include "entity/string/string_helper.h"
#include "entity/tuple/tuple.h"
#include "vm/module_manager.h"
#include "vm/process/processes.h"
#include "vm/vm.h"

Entity _open_c_lib(Task *task, Context *ctx, Object *obj, Entity *args) {
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
  if (!IS_STRING(arg2)) {
    return raise_error(task, ctx,
                       "Invalid argument(2) for "
                       "__open_c_lib: Expected type String.");
  }

  char *file_name = entity_string_copy(arg0);
  char *module_name = entity_string_copy(arg1);
  char *init_fn_name = entity_string_copy(arg2);

#ifdef OS_WINDOWS
  HMODULE dl_handle = LoadLibrary(TEXT(file_name));
#else
  void *dl_handle = dlopen(file_name, RTLD_LAZY);
#endif
  if (NULL == dl_handle) {
#ifdef OS_WINDOWS
    return raise_error(task, ctx, "Invalid file_name for library: %s",
                       file_name);
#else
    return raise_error(task, ctx, "Invalid file_name for library: %s",
                       dlerror());
#endif
  }

#ifdef OS_WINDOWS
  NativeCallback init_fn =
      (NativeCallback)GetProcAddress(dl_handle, init_fn_name);
#else
  NativeCallback init_fn = (NativeCallback)dlsym(dl_handle, init_fn_name);
#endif

  if (NULL == init_fn) {
    RELEASE(file_name);
    RELEASE(module_name);
    RELEASE(init_fn_name);
#ifdef OS_WINDOWS
    return raise_error(task, ctx, "'%s' not found in library: %s.",
                       init_fn_name, file_name);
#else
    return raise_error(task, ctx, "'%s' not found in library: %s", init_fn_name,
                       dlerror());

#endif
  }

  ModuleManager *mm = vm_module_manager(task->parent_process->vm);
  mm_register_dynamic_module(mm, module_name, init_fn);
  // Forces module to be loaded eagerly.
  Module *module = modulemanager_lookup(mm, module_name);
  RELEASE(file_name);
  RELEASE(module_name);
  RELEASE(init_fn_name);
  return entity_object(module->_reflection);
}

void dynamic_add_native(ModuleManager *mm, Module *dynamic) {
  native_function(dynamic, intern("__open_c_lib"), _open_c_lib);
}