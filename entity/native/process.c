// process.c
//
// Created on: Dec 28, 2020
//     Author: Jeff Manzione

#include "entity/native/process.h"

#include "util/platform.h"

#ifdef OS_WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef OS_WINDOWS
#include <synchapi.h>
#endif

#include "alloc/arena/intern.h"
#include "entity/class/class.h"
#include "entity/class/classes_def.h"
#include "entity/native/error.h"
#include "entity/native/native.h"
#include "entity/object.h"
#include "entity/tuple/tuple.h"
#include "struct/struct_defaults.h"
#include "util/sync/mutex.h"
#include "util/sync/thread.h"
#include "vm/intern.h"
#include "vm/process/context.h"
#include "vm/process/process.h"
#include "vm/process/processes.h"
#include "vm/process/task.h"
#include "vm/vm.h"

// void _remote_init(Object *obj) {}
// void _remote_delete(Object *obj) {}

Entity _create_process(Task *current_task, Context *current_ctx, Object *obj,
                       Entity *args) {
  if (!IS_CLASS(args, Class_Tuple)) {
    return raise_error(
        current_task, current_ctx,
        "Cannot call create_process with something other than (Function, "
        "ANY).");
  }
  Tuple *tuple = (Tuple *)args->obj->_internal_obj;
  if (2 != tuple_size(tuple)) {
    return raise_error(current_task, current_ctx,
                       "create_process expects 2 args.");
  }
  const Entity *fn = tuple_get(tuple, 0);
  const Entity *fn_args = tuple_get(tuple, 1);
  if (!IS_OBJECT(fn) || !(inherits_from(fn->obj->_class, Class_Function) ||
                          IS_CLASS(fn, Class_FunctionRef))) {
    return raise_error(current_task, current_ctx,
                       "create_processes expects (Function, ANY).");
  }

  VM *vm = current_task->parent_process->vm;
  Process *new_process = vm_create_process(vm);
  Task *new_task = process_create_task(new_process);
  new_task->parent_task = current_task;
  Context *new_ctx;

  const Function *f;
  Object *self;
  if (IS_CLASS(fn, Class_Function)) {
    f = fn->obj->_function_obj;
    self = f->_module->_reflection;
  } else if (IS_CLASS(fn, Class_FunctionRef)) {
    f = function_ref_get_func(fn->obj);
    self = function_ref_get_object(fn->obj);
  } else {
    return raise_error(current_task, current_ctx,
                       "create_processes expects (Function, ANY).");
  }

  Map cps;
  map_init_default(&cps);
  // SYNCHRONIZED(current_task->parent_process->heap_access_lock, {
  Entity self_e = entity_object(self);
  self = entity_copy(new_process->heap, &cps, &self_e).obj;
  new_ctx =
      task_create_context(new_task, self, (Module *)f->_module, f->_ins_pos);
  *task_mutable_resval(new_task) =
      entity_copy(new_process->heap, &cps, fn_args);
  // });
  map_finalize(&cps);

  context_set_function(new_ctx, f);
  return entity_object(new_process->_reflection);
}

Entity __sleep(Task *task, Context *ctx, Object *obj, Entity *args) {
  double sleep_duration_sec = 0;
  if (IS_INT(args)) {
    sleep_duration_sec = pint(&args->pri);
  } else if (IS_FLOAT(args)) {
    sleep_duration_sec = pfloat(&args->pri);
  } else {
    return raise_error(task, ctx, "sleep() expected to be called with number.");
  }
#if defined(OS_WINDOWS)
  // Accepts millis as an unsigned long instead of double seconds.
  Sleep(sleep_duration_sec * 1000);
#else
  sleep(sleep_duration_sec);
#endif
  return NONE_ENTITY;
}

void process_add_native(ModuleManager *mm, Module *process) {
  // Class_Remote =
  //     native_class(process, REMOTE_CLASS_NAME, _remote_init,
  //     _remote_delete);
  native_function(process, intern("__create_process"), _create_process);
  native_background_function(process, intern("__sleep"), __sleep);
}
