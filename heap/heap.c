// heap.h
//
// Created on: May 28, 2020
//     Author: Jeff Manzione

#include "heap/heap.h"

#include "alloc/alloc.h"
#include "alloc/arena/arena.h"
#include "alloc/memory_graph/memory_graph.h"
#include "debug/debug.h"
#include "entity/array/array.h"
#include "entity/object.h"
#include "entity/tuple/tuple.h"
#include "struct/alist.h"
#include "struct/keyed_list.h"

struct _Heap {
  MGraph *mg;
  __Arena object_arena;
};

Object *_object_create(Heap *heap, const Class *class);
void _object_delete(Object *object, Heap *heap);

Heap *heap_create(HeapConf *config) {
  ASSERT(NOT_NULL(config));
  Heap *heap = ALLOC2(Heap);
  config->mgraph_config.ctx = heap;
  heap->mg = mgraph_create(&config->mgraph_config);
  __arena_init(&heap->object_arena, sizeof(Object), "Object");
  return heap;
}

void heap_delete(Heap *heap) {
  ASSERT(NOT_NULL(heap), NOT_NULL(heap->mg));
  mgraph_delete(heap->mg);
  __arena_finalize(&heap->object_arena);
  DEALLOC(heap);
}

Object *heap_new(Heap *heap, const Class *class) {
  ASSERT(NOT_NULL(heap), NOT_NULL(class));
  Object *object = _object_create(heap, class);
  // Blessed
  object->_node_ref = mgraph_insert(heap->mg, object, (Deleter)_object_delete);
  return object;
}

void object_set_member(Heap *heap, Object *parent, const char key[],
                       const Entity *child) {
  ASSERT(NOT_NULL(heap), NOT_NULL(parent), NOT_NULL(child));
  Entity *entry_pos;
  Entity *old_member =
      (Entity *)keyedlist_insert(&parent->_members, key, (void **)&entry_pos);
  ASSERT(NOT_NULL(entry_pos));
  if (NULL != old_member && OBJECT == etype(old_member)) {
    mgraph_dec(heap->mg, (Node *)parent->_node_ref,
               (Node *)object_m(old_member)->_node_ref);
  }
  if (OBJECT == etype(child)) {
    mgraph_inc(heap->mg, (Node *)parent->_node_ref,
               (Node *)object(child)->_node_ref);
  }
  (*entry_pos) = *child;
}

Entity *object_set_member_obj(Heap *heap, Object *parent, const char key[],
                              const Object *child) {
  ASSERT(NOT_NULL(heap), NOT_NULL(parent), NOT_NULL(child));
  Entity *entry_pos;
  Entity *old_member =
      (Entity *)keyedlist_insert(&parent->_members, key, (void **)&entry_pos);
  ASSERT(NOT_NULL(entry_pos));
  if (NULL != old_member && OBJECT == etype(old_member)) {
    mgraph_dec(heap->mg, (Node *)parent->_node_ref,
               (Node *)object_m(old_member)->_node_ref);
  }
  mgraph_inc(heap->mg, (Node *)parent->_node_ref, (Node *)child->_node_ref);
  entry_pos->type = OBJECT;
  entry_pos->obj = (Object *)child;
  return entry_pos;
}

Object *_object_create(Heap *heap, const Class *class) {
  ASSERT(NOT_NULL(heap));
  Object *object = (Object *)__arena_alloc(&heap->object_arena);
  object->_class = class;
  keyedlist_init(&object->_members, Entity, DEFAULT_ARRAY_SZ);
  if (NULL != class->_init_fn) {
    class->_init_fn(object);
  }
  return object;
}

void _object_delete(Object *object, Heap *heap) {
  ASSERT(NOT_NULL(heap), NOT_NULL(object));
  if (NULL != object && NULL != object->_class->_delete_fn) {
    object->_class->_delete_fn(object);
  }
  keyedlist_finalize(&object->_members);
  __arena_dealloc(&heap->object_arena, object);
}

void array_add(Heap *heap, Object *array, const Entity *child) {
  ASSERT(NOT_NULL(heap), NOT_NULL(array), NOT_NULL(child));
  Entity *e = Array_add_last((Array *)array->_internal_obj);
  *e = *child;
  if (OBJECT != child->type) {
    return;
  }
  mgraph_inc(heap->mg, (Node *)array->_node_ref, (Node *)child->obj->_node_ref);
}

// Does this need to handle overwrites?
void tuple_set(Heap *heap, Object *array, uint32_t index, const Entity *child) {
  ASSERT(NOT_NULL(heap), NOT_NULL(array), NOT_NULL(child));
  ASSERT(index >= 0, index < tuple_size((Tuple *)array->_internal_obj));
  // bless
  Entity *e = tuple_get_mutable((Tuple *)array->_internal_obj, index);
  *e = *child;
  if (OBJECT != child->type) {
    return;
  }
  mgraph_inc(heap->mg, (Node *)array->_node_ref, (Node *)child->obj->_node_ref);
}