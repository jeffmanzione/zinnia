// heap.h
//
// Created on: May 28, 2020
//     Author: Jeff Manzione

#ifndef HEAP_HEAP_H_
#define HEAP_HEAP_H_

#include "entity/entity.h"
#include "entity/object.h"

typedef struct _Heap Heap;

typedef struct {
  MGraphConf mgraph_config;
} HeapConf;

Heap *heap_create(HeapConf *config);
void heap_delete(Heap *heap);
Object *heap_new(Heap *heap, const Class *class);

void object_set_member(Heap *heap, Object *parent, const char key[],
                       Entity *child);

#endif /* HEAP_HEAP_H_ */