// heap.h
//
// Created on: May 28, 2020
//     Author: Jeff Manzione

#include "heap/heap.h"

#include "alloc/alloc.h"

struct _Heap {};

Heap *heap_create(HeapConf *config) { Heap *heap = ALLOC2(Heap); }

void heap_delete(Heap *heap) { DEALLOC(heap); }

Object *heap_new(Heap *heap, Class *class) {}