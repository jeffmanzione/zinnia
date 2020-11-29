// async.h
//
// Created on: Nov 11, 2020
//     Author: Jeff Manzione

#ifndef ENTITY_NATIVE_ASYNC_H_
#define ENTITY_NATIVE_ASYNC_H_

#include "entity/object.h"
#include "heap/heap.h"
#include "vm/process/processes.h"

typedef struct {
  Task *task;
} Future;

Object *future_create(Task *task);

void async_add_native(Module *async);

#endif /* ENTITY_NATIVE_BUILTIN_H_ */