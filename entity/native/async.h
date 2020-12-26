// async.h
//
// Created on: Nov 11, 2020
//     Author: Jeff Manzione

#ifndef ENTITY_NATIVE_ASYNC_H_
#define ENTITY_NATIVE_ASYNC_H_

#include "entity/object.h"
#include "heap/heap.h"
#include "vm/process/processes.h"

typedef struct _Future Future;

Object *future_create(Task *task);
bool future_is_complete(Future *f);
const Entity *future_get_value(Heap *heap, Object *obj);
Task *future_get_task(Future *f);

void async_add_native(Module *async);

#endif /* ENTITY_NATIVE_BUILTIN_H_ */