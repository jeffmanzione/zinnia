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
#include "entity/string/string.h"
#include "entity/tuple/tuple.h"
#include "struct/struct_defaults.h"
#include "util/sync/mutex.h"
#include "util/sync/thread.h"
#include "vm/intern.h"
#include "vm/process/context.h"
#include "vm/process/process.h"
#include "vm/process/processes.h"
#include "vm/process/remote.h"
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
        "Cannot call __create_process with something other than (Function, "
        "ANY, ANY).");
  }
  Tuple *tuple = (Tuple *)args->obj->_internal_obj;
  if (3 != tuple_size(tuple)) {
    return raise_error(current_task, current_ctx,
                       "__create_process expects 3 args.");
  }
  const Entity *fn = tuple_get(tuple, 0);
  const Entity *fn_args = tuple_get(tuple, 1);
  const Entity *is_remote_arg = tuple_get(tuple, 2);
  if (!IS_OBJECT(fn) || !(inherits_from(fn->obj->_class, Class_Function) ||
                          IS_CLASS(fn, Class_FunctionRef))) {
    return raise_error(current_task, current_ctx,
                       "__create_processes expects (Function, ANY, ANY).");
  }

  VM *vm = current_task->parent_process->vm;
  Process *new_process = vm_create_process(vm);
  new_process->is_remote = !IS_NONE(is_remote_arg);
  Task *new_task = process_create_task(new_process);
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
                       "__create_processes expects (Function, ANY).");
  }

  Map cps;
  map_init_default(&cps);
  Entity self_e = entity_object(self);
  self = entity_copy(new_process->heap, &cps, &self_e).obj;
  new_ctx =
      task_create_context(new_task, self, (Module *)f->_module, f->_ins_pos);
  *task_mutable_resval(new_task) =
      entity_copy(new_process->heap, &cps, fn_args);
  map_finalize(&cps);

  context_set_function(new_ctx, f);

  // Ensures that the remote task does not end while it is still accessible.
  if (new_process->is_remote) {
    new_process->remote_non_daemon_task =
        process_create_unqueued_task(new_process);
    new_process->remote_non_daemon_task->state = TASK_WAITING;
    process_insert_waiting_task(new_process,
                                new_process->remote_non_daemon_task);
  }

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

Entity _validate_remote_call(Task *current_task, Context *current_ctx,
                             Entity *args) {
  if (!IS_CLASS(args, Class_Tuple)) {
    return raise_error(current_task, current_ctx,
                       "Cannot call __remote_call with something other than "
                       "(Remote, String, args).");
  }
  Tuple *tuple = (Tuple *)args->obj->_internal_obj;
  if (3 != tuple_size(tuple)) {
    return raise_error(current_task, current_ctx,
                       "__remote_call expects 2 args.");
  }
  const Entity *remote_entity = tuple_get(tuple, 0);
  if (!IS_CLASS(remote_entity, Class_Remote)) {
    return raise_error(current_task, current_ctx,
                       "__remote_call expects (Remote, String, args).");
  }

  const Entity *fn_name_entity = tuple_get(tuple, 1);
  if (!IS_CLASS(fn_name_entity, Class_String)) {
    return raise_error(current_task, current_ctx,
                       "__remote_call expects (Remote, String, args).");
  }
  return NONE_ENTITY;
}

Entity _create_future_for_task(Task *current_task, Task *remote_task) {
  Task *new_task = process_create_unqueued_task(current_task->parent_process);
  new_task->parent_task = current_task;

  Object *future_obj = future_create(new_task);
  remote_task->remote_future = (Future *)future_obj->_internal_obj;

  new_task->state = TASK_WAITING;
  process_insert_waiting_task(current_task->parent_process, new_task);

  return entity_object(future_obj);
}

Entity _remote_call(Task *current_task, Context *current_ctx, Object *obj,
                    Entity *args) {
  Entity error = _validate_remote_call(current_task, current_ctx, args);
  if (NONE != error.type) {
    return error;
  }

  const Tuple *tuple = (Tuple *)args->obj->_internal_obj;
  const Entity *remote_entity = tuple_get(tuple, 0);
  const Entity *fn_name_entity = tuple_get(tuple, 1);
  const Entity *fn_args = tuple_get(tuple, 2);

  Remote *remote = extract_remote_from_obj(remote_entity->obj);
  Process *remote_process = remote_get_process(remote);
  Object *remote_object = remote_get_object(remote);

  String *fn_name_string = (String *)fn_name_entity->obj->_internal_obj;
  const char *fn_name =
      intern_range(fn_name_string->table, 0, String_size(fn_name_string));

  Entity fn =
      object_get_maybe_wrap(remote_object, fn_name, remote_process->heap, NULL);

  const Function *f;
  Object *self;
  if (IS_CLASS(&fn, Class_Function)) {
    f = fn.obj->_function_obj;
    self = f->_module->_reflection;
  } else if (IS_CLASS(&fn, Class_FunctionRef)) {
    f = function_ref_get_func(fn.obj);
    self = function_ref_get_object(fn.obj);
  } else {
    return raise_error(current_task, current_ctx,
                       "Could not find method '%s' on remote object.", fn_name);
  }

  Task *remote_task = process_create_unqueued_task(remote_process);

  Context *remote_ctx = task_create_context(remote_task, remote_object,
                                            (Module *)f->_module, f->_ins_pos);
  context_set_function(remote_ctx, f);

  Map cps;
  map_init_default(&cps);
  SYNCHRONIZED(current_task->parent_process->heap_access_lock, {
    SYNCHRONIZED(remote_process->heap_access_lock, {
      *task_mutable_resval(remote_task) =
          entity_copy(remote_process->heap, &cps, fn_args);
    });
  });
  map_finalize(&cps);

  process_enqueue_task(remote_process, remote_task);

  Entity future_entity = _create_future_for_task(current_task, remote_task);
  condition_broadcast(remote_process->task_wait_cond);
  return future_entity;
}

void async_add_native(ModuleManager *mm, Module *async) {
  Class_Future = native_class(async, FUTURE_NAME, _future_init, _future_delete);
  native_function(async, VALUE_KEY, _future_value);
  native_function(async, intern("__create_process"), _create_process);
  native_background_function(async, intern("__sleep"), __sleep);
  native_function(async, intern("__remote_call"), _remote_call);
}