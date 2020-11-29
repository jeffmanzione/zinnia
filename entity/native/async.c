// async.c
//
// Created on: Nov 11, 2020
//     Author: Jeff Manzione

#include "entity/native/async.h"

#include "entity/class/classes.h"
#include "entity/native/native.h"
#include "vm/intern.h"

void _future_init(Object *obj) {
  obj->_internal_obj = ALLOC2(Future);
}

void _future_delete(Object *obj) {
  DEALLOC(obj->_internal_obj);
}

Object *future_create(Task *task) {
  Heap *heap = task->parent_process->heap;
  Object *future_obj = heap_new(heap, Class_Future);
  Future *future = (Future *) future_obj->_internal_obj;
  future->task = task;
  return future_obj;
}

void async_add_native(Module *async) {
  Class_Future = native_class(async, FUTURE_NAME, _future_init, _future_delete);
}