// async.c
//
// Created on: Nov 11, 2020
//     Author: Jeff Manzione

#include "entity/native/async.h"

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

struct _Future {
  Task *task;
  bool _is_complete, _is_result_set;
};

void _future_init(Object *obj) {
  Future *f = ALLOC2(Future);
  f->_is_complete = false;
  f->_is_result_set = false;
  f->task = NULL;
  obj->_internal_obj = f;
}

void _future_delete(Object *obj) { DEALLOC(obj->_internal_obj); }

Object *future_create(Task *task) {
  Heap *heap = task->parent_process->heap;
  Object *future_obj = heap_new(heap, Class_Future);
  Future *future = (Future *)future_obj->_internal_obj;
  future->task = task;
  return future_obj;
}

bool future_is_complete(Future *f) {
  if (f->_is_complete) {
    return true;
  }
  f->_is_complete =
      (f->task->state == TASK_COMPLETE) || (f->task->state == TASK_ERROR);
  return f->_is_complete;
}

Entity _future_value(Task *task, Context *ctx, Object *obj, Entity *args) {
  Object *f_obj = heap_new(task->parent_process->heap, Class_Future);
  Future *f = (Future *)f_obj->_internal_obj;
  f->_is_complete = true;
  f->_is_result_set = true;
  object_set_member(task->parent_process->heap, f_obj, RESULT_VAL, args);
  return entity_object(f_obj);
}

const Entity *future_get_value(Heap *heap, Object *obj) {
  ASSERT(NOT_NULL(obj));
  ASSERT(obj->_class == Class_Future);
  Future *f = (Future *)obj->_internal_obj;
  if (!f->_is_result_set) {
    const Entity *result = task_get_resval(f->task);
    object_set_member(heap, obj, RESULT_VAL, result);
    f->_is_result_set = true;
    return result;
  }
  return object_get(obj, RESULT_VAL);
}

Task *future_get_task(Future *f) { return f->task; }

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

void async_add_native(ModuleManager *mm, Module *async) {
  Class_Future = native_class(async, FUTURE_NAME, _future_init, _future_delete);
  native_function(async, VALUE_KEY, _future_value);
  native_function(async, intern("__create_process"), _create_process);
  native_background_function(async, intern("__sleep"), __sleep);
}