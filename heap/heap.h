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
uint32_t heap_collect_garbage(Heap *heap);
void heap_make_root(Heap *heap, Object *obj);

void heap_inc_edge(Heap *heap, Object *parent, Object *child);
void heap_dec_edge(Heap *heap, Object *parent, Object *child);

void object_set_member(Heap *heap, Object *parent, const char key[],
                       const Entity *child);
Entity *object_set_member_obj(Heap *heap, Object *parent, const char key[],
                              const Object *child);

void array_add(Heap *heap, Object *array, const Entity *child);
void array_set(Heap *heap, Object *array, uint32_t index, const Entity *child);
void tuple_set(Heap *heap, Object *array, uint32_t index, const Entity *child);

#endif /* HEAP_HEAP_H_ */