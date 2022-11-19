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
  uint32_t max_object_count;
} HeapConf;

Heap *heap_create(HeapConf *config);
void heap_delete(Heap *heap);
Object *heap_new(Heap *heap, const Class *class);
uint32_t heap_collect_garbage(Heap *heap);
void heap_print_debug_summary(Heap *heap);
uint32_t heap_object_count(const Heap *const heap);
uint32_t heap_max_object_count(const Heap *const heap);
uint32_t
heap_object_count_threshold_for_garbage_collection(const Heap *const heap);
void heap_set_object_count_threshold_for_garbage_collection(
    Heap *heap, uint32_t new_threshold);
void heap_make_root(Heap *heap, Object *obj);

void heap_inc_edge(Heap *heap, Object *parent, Object *child);
void heap_dec_edge(Heap *heap, Object *parent, Object *child);

void object_set_member(Heap *heap, Object *parent, const char key[],
                       const Entity *child);
Entity *object_set_member_obj(Heap *heap, Object *parent, const char key[],
                              const Object *child);

void array_add(Heap *heap, Object *array, const Entity *child);
Entity array_remove(Heap *heap, Object *array, int32_t index);
void array_set(Heap *heap, Object *array, int32_t index, const Entity *child);
Object *array_create(Heap *heap);

void tuple_set(Heap *heap, Object *array, int32_t index, const Entity *child);
Object *tuple_create2(Heap *heap, Entity *e1, Entity *e2);
Object *tuple_create3(Heap *heap, Entity *e1, Entity *e2, Entity *e3);
Object *tuple_create4(Heap *heap, Entity *e1, Entity *e2, Entity *e3,
                      Entity *e4);
Object *tuple_create5(Heap *heap, Entity *e1, Entity *e2, Entity *e3,
                      Entity *e4, Entity *e5);
Object *tuple_create6(Heap *heap, Entity *e1, Entity *e2, Entity *e3,
                      Entity *e4, Entity *e5, Entity *e6);
Object *tuple_create7(Heap *heap, Entity *e1, Entity *e2, Entity *e3,
                      Entity *e4, Entity *e5, Entity *e6, Entity *e7);

void named_list_set(Heap *heap, Object *obj, const char key[],
                    const Entity *new);

typedef struct _HeapProfile HeapProfile;

HeapProfile *heap_create_profile(const Heap *const heap);
M_iter heapprofile_object_type_counts(const HeapProfile *const hp);
void heapprofile_delete(HeapProfile *hp);

Entity entity_copy(Heap *heap, Map *copy_map, const Entity *e);

#endif /* HEAP_HEAP_H_ */