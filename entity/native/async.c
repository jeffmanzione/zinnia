// async.c
//
// Created on: Nov 11, 2020
//     Author: Jeff Manzione

#include "entity/native/async.h"

#include "entity/class/classes_def.h"
#include "entity/native/native.h"
#include "vm/intern.h"
#include "vm/process/task.h"

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

void async_add_native(ModuleManager *mm, Module *async) {
  Class_Future = native_class(async, FUTURE_NAME, _future_init, _future_delete);
  native_function(async, VALUE_KEY, _future_value);
}