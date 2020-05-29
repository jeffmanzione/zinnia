// heap.h
//
// Created on: May 28, 2020
//     Author: Jeff Manzione

#ifndef HEAP_HEAP_H_
#define HEAP_HEAP_H_

#include "object/object.h"

typedef struct _Heap Heap;

typedef struct {
} HeapConf;

Heap *heap_create(HeapConf *config);
void heap_delete(Heap *heap);
Object *heap_new(Heap *heap, Class *class);

#endif /* HEAP_HEAP_H_ */