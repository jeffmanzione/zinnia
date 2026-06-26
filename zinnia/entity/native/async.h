// async.h
//
// Created on: Nov 11, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_ASYNC_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_ASYNC_H_

#include "zinnia/entity/object.h"
#include "zinnia/heap/heap.h"
#include "zinnia/vm/module_manager.h"
#include "zinnia/vm/process/processes.h"

typedef struct Future_ Future;

Object *future_create(Task *task);
bool future_is_complete(Future *f);
const Entity *future_get_value(Heap *heap, Object *obj);
Task *future_get_task(Future *f);

void async_add_native(ModuleManager *mm, Module *async);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_ASYNC_H_ */