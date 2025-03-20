// heap.h
//
// Created on: May 28, 2020
//     Author: Jeff Manzione

#include "heap/heap.h"

#include <stdint.h>

#include "alloc/alloc.h"
#include "alloc/arena/arena.h"
#include "alloc/memory_graph/memory_graph.h"
#include "entity/array/array.h"
#include "entity/class/classes_def.h"
#include "entity/object.h"
#include "entity/string/string.h"
#include "entity/tuple/tuple.h"
#include "struct/alist.h"
#include "struct/keyed_list.h"
#include "struct/struct_defaults.h"

struct _Heap {
  HeapConf config;
  MGraph *mg;
  __Arena object_arena;
  uint32_t object_count_threshold_for_garbage_collection;
};

struct _HeapProfile {
  Map object_type_counts;
};

Object *_object_create(Heap *heap, const Class *class);
void _object_delete(Object *object, Heap *heap);

Heap *heap_create(HeapConf *config) {
  ASSERT(NOT_NULL(config));
  Heap *heap = ALLOC2(Heap);
  config->mgraph_config.ctx = heap;
  heap->config = *config;
  heap->object_count_threshold_for_garbage_collection =
      config->max_object_count / 2;
  heap->mg = mgraph_create(&heap->config.mgraph_config);
  __arena_init(&heap->object_arena, sizeof(Object), "Object");
  return heap;
}

void heap_delete(Heap *heap) {
  ASSERT(NOT_NULL(heap), NOT_NULL(heap->mg));
  mgraph_delete(heap->mg);
  __arena_finalize(&heap->object_arena);
  DEALLOC(heap);
}

uint32_t heap_collect_garbage(Heap *heap) {
  return mgraph_collect_garbage(heap->mg);
}

uint32_t heap_object_count(const Heap *const heap) {
  ASSERT(NOT_NULL(heap));
  return mgraph_node_count(heap->mg);
}

uint32_t heap_max_object_count(const Heap *const heap) {
  ASSERT(NOT_NULL(heap));
  return heap->config.max_object_count;
}

uint32_t
heap_object_count_threshold_for_garbage_collection(const Heap *const heap) {
  ASSERT(NOT_NULL(heap));
  return heap->object_count_threshold_for_garbage_collection;
}

void heap_set_object_count_threshold_for_garbage_collection(
    Heap *heap, uint32_t new_threshold) {
  ASSERT(NOT_NULL(heap));
  heap->object_count_threshold_for_garbage_collection = new_threshold;
}

DEB_FN(Object *, heap_new, Heap *heap, const Class *class) {
  ASSERT(NOT_NULL(heap));
  ASSERT(NOT_NULL(class));
  Object *object = _object_create(heap, class);
  object->_node_ref = mgraph_insert(heap->mg, object, (Deleter)_object_delete);
  return object;
}

void heap_make_root(Heap *heap, Object *obj) {
  mgraph_root(heap->mg, (Node *)obj->_node_ref);
}

void object_set_member(Heap *heap, Object *parent, const char key[],
                       const Entity *child) {
  // if (0 == strcmp("sock", key)) {
  //   printf("--------->ctx(%p@%p).sock=(%p)\n", parent, heap, child->obj);
  // }
  ASSERT(NOT_NULL(heap), NOT_NULL(parent), NOT_NULL(child));
  Entity *entry_pos;
  Entity *old_member =
      &((Member *)keyedlist_insert(&parent->_members, key, (void **)&entry_pos))
           ->entity;
  ASSERT(NOT_NULL(entry_pos));
  const bool old_member_is_obj =
      NULL != old_member && OBJECT == etype(old_member);
  if (old_member_is_obj && (old_member->obj == child->obj)) {
    return;
  }
  if (old_member_is_obj) {
    heap_dec_edge(heap, parent, object_m(old_member));
  }
  if (OBJECT == etype(child)) {
    heap_inc_edge(heap, parent, object_m((Entity *)child));
  }
  (*entry_pos) = *child;
}

Entity *object_set_member_obj(Heap *heap, Object *parent, const char key[],
                              const Object *child) {
  ASSERT(NOT_NULL(heap), NOT_NULL(parent), NOT_NULL(child));
  Entity *entry_pos;
  Entity *old_member =
      &((Member *)keyedlist_insert(&parent->_members, key, (void **)&entry_pos))
           ->entity;
  ASSERT(NOT_NULL(entry_pos));
  const bool old_member_is_obj =
      NULL != old_member && OBJECT == etype(old_member);
  if (old_member_is_obj && old_member->obj == child) {
    return entry_pos;
  }
  if (old_member_is_obj) {
    heap_dec_edge(heap, parent, object_m(old_member));
  }
  heap_inc_edge(heap, parent, (Object *)child);
  entry_pos->type = OBJECT;
  entry_pos->obj = (Object *)child;
  return entry_pos;
}

Object *_object_create(Heap *heap, const Class *class) {
  ASSERT(NOT_NULL(heap));
  Object *object = (Object *)__arena_alloc(&heap->object_arena);
  object->_class = class;
  keyedlist_init(&object->_members, Member, DEFAULT_ARRAY_SZ);
  if (NULL != class->_init_fn) {
    class->_init_fn(object);
  }
  return object;
}

void _print_object_summary(Object *object) {
  ASSERT(NOT_NULL(object));
  if (object->_class == Class_Class) {
    printf("\tClass('%s')", object->_class_obj->_name);
  } else if (object->_class == Class_Module) {
    printf("\tModule('%s')", object->_module_obj->_name);
  } else if (object->_class == Class_Function) {
    printf("\tFunction('%s')", object->_function_obj->_name);
  } else if (object->_class == Class_String) {
    printf("\tString('%.*s', %p)",
           String_size(((String *)object->_internal_obj)),
           ((String *)object->_internal_obj)->table, object->_internal_obj);
  } else {
    printf("\t%s", object->_class->_name);
  }
  printf(" (%p)\n", object);

  // KL_iter members = keyedlist_iter(&object->_members);
  // for (; kl_has(&members); kl_inc(&members)) {
  //   const Member *member = (Member *)kl_value(&members);
  // if (NULL != member && OBJECT == member->entity.type) {
  //   printf("\t\t%s(%p)\n", (char *)kl_key(&members), member->entity.obj);
  // }
  // }
  // fflush(stdout);
}

void heap_print_debug_summary(Heap *heap) {
  const Set *nodes = mgraph_nodes(heap->mg);
  M_iter iter = set_iter((Set *)nodes);
  for (; has(&iter); inc(&iter)) {
    const Node *node = (Node *)value(&iter);
    Object *obj = (Object *)node_ptr(node);
    _print_object_summary(obj);
  }
}

void _object_delete(Object *object, Heap *heap) {
  ASSERT(NOT_NULL(heap), NOT_NULL(object));
  // _print_object_summary(object);
  if (NULL != object->_class->_delete_fn) {
    object->_class->_delete_fn(object);
  }
  keyedlist_finalize(&object->_members);
  __arena_dealloc(&heap->object_arena, object);
}

void heap_inc_edge(Heap *heap, Object *parent, Object *child) {
  ASSERT(NOT_NULL(heap), NOT_NULL(parent), NOT_NULL(child));
  mgraph_inc(heap->mg, (Node *)parent->_node_ref, (Node *)child->_node_ref);
}

void heap_dec_edge(Heap *heap, Object *parent, Object *child) {
  ASSERT(NOT_NULL(heap), NOT_NULL(parent), NOT_NULL(child));
  mgraph_dec(heap->mg, (Node *)parent->_node_ref, (Node *)child->_node_ref);
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

Entity array_remove(Heap *heap, Object *array, int32_t index) {
  ASSERT(NOT_NULL(heap), NOT_NULL(array), index >= 0);
  Entity e = Array_remove((Array *)array->_internal_obj, index);
  if (OBJECT == e.type) {
    heap_dec_edge(heap, array, e.obj);
  }
  return e;
}

void array_set(Heap *heap, Object *array, int32_t index, const Entity *child) {
  ASSERT(NOT_NULL(heap), NOT_NULL(array), NOT_NULL(child), index >= 0);
  Entity *e = Array_set_ref((Array *)array->_internal_obj, index);
  if (NULL != e && OBJECT == e->type) {
    heap_dec_edge(heap, array, e->obj);
  }
  *e = *child;
  if (OBJECT != child->type) {
    return;
  }
  heap_inc_edge(heap, array, child->obj);
}

Object *array_create(Heap *heap) { return heap_new(heap, Class_Array); }

// This function does not handle overwrites, so references to overwritten
// members will persist.
void tuple_set(Heap *heap, Object *tuple, int32_t index, const Entity *child) {
  ASSERT(NOT_NULL(heap), NOT_NULL(tuple), NOT_NULL(child));
  ASSERT(index >= 0, index < tuple_size((Tuple *)tuple->_internal_obj));
  Entity *e = tuple_get_mutable((Tuple *)tuple->_internal_obj, index);
  *e = *child;
  if (OBJECT != child->type) {
    return;
  }
  heap_inc_edge(heap, tuple, child->obj);
}

Object *tuple_create2(Heap *heap, Entity *e1, Entity *e2) {
  Object *tuple_obj = heap_new(heap, Class_Tuple);
  tuple_obj->_internal_obj = tuple_create(2);
  tuple_set(heap, tuple_obj, 0, e1);
  tuple_set(heap, tuple_obj, 1, e2);
  return tuple_obj;
}

Object *tuple_create3(Heap *heap, Entity *e1, Entity *e2, Entity *e3) {
  Object *tuple_obj = heap_new(heap, Class_Tuple);
  tuple_obj->_internal_obj = tuple_create(3);
  tuple_set(heap, tuple_obj, 0, e1);
  tuple_set(heap, tuple_obj, 1, e2);
  tuple_set(heap, tuple_obj, 2, e3);
  return tuple_obj;
}

Object *tuple_create4(Heap *heap, Entity *e1, Entity *e2, Entity *e3,
                      Entity *e4) {
  Object *tuple_obj = heap_new(heap, Class_Tuple);
  tuple_obj->_internal_obj = tuple_create(4);
  tuple_set(heap, tuple_obj, 0, e1);
  tuple_set(heap, tuple_obj, 1, e2);
  tuple_set(heap, tuple_obj, 2, e3);
  tuple_set(heap, tuple_obj, 3, e4);
  return tuple_obj;
}

Object *tuple_create5(Heap *heap, Entity *e1, Entity *e2, Entity *e3,
                      Entity *e4, Entity *e5) {
  Object *tuple_obj = heap_new(heap, Class_Tuple);
  tuple_obj->_internal_obj = tuple_create(5);
  tuple_set(heap, tuple_obj, 0, e1);
  tuple_set(heap, tuple_obj, 1, e2);
  tuple_set(heap, tuple_obj, 2, e3);
  tuple_set(heap, tuple_obj, 3, e4);
  tuple_set(heap, tuple_obj, 4, e5);
  return tuple_obj;
}

Object *tuple_create6(Heap *heap, Entity *e1, Entity *e2, Entity *e3,
                      Entity *e4, Entity *e5, Entity *e6) {
  Object *tuple_obj = heap_new(heap, Class_Tuple);
  tuple_obj->_internal_obj = tuple_create(6);
  tuple_set(heap, tuple_obj, 0, e1);
  tuple_set(heap, tuple_obj, 1, e2);
  tuple_set(heap, tuple_obj, 2, e3);
  tuple_set(heap, tuple_obj, 3, e4);
  tuple_set(heap, tuple_obj, 4, e5);
  tuple_set(heap, tuple_obj, 5, e6);
  return tuple_obj;
}

Object *tuple_create7(Heap *heap, Entity *e1, Entity *e2, Entity *e3,
                      Entity *e4, Entity *e5, Entity *e6, Entity *e7) {
  Object *tuple_obj = heap_new(heap, Class_Tuple);
  tuple_obj->_internal_obj = tuple_create(7);
  tuple_set(heap, tuple_obj, 0, e1);
  tuple_set(heap, tuple_obj, 1, e2);
  tuple_set(heap, tuple_obj, 2, e3);
  tuple_set(heap, tuple_obj, 3, e4);
  tuple_set(heap, tuple_obj, 4, e5);
  tuple_set(heap, tuple_obj, 5, e6);
  tuple_set(heap, tuple_obj, 6, e7);
  return tuple_obj;
}

void entitycopier_init(EntityCopier *copier, Heap *target) {
  ASSERT(NOT_NULL(copier), NOT_NULL(target));
  copier->target = target;
  map_init_default(&copier->copy_map);
}

void entitycopier_finalize(EntityCopier *copier) {
  ASSERT(NOT_NULL(copier));
  map_finalize(&copier->copy_map);
}

Entity entitycopier_copy(EntityCopier *copier, const Entity *e) {
  ASSERT(NOT_NULL(copier), NOT_NULL(e));
  ASSERT(NOT_NULL(e));
  switch (e->type) {
  case NONE:
  case PRIMITIVE:
    return *e;
  default:
    ASSERT(OBJECT == e->type);
  }
  Object *obj = e->obj;
  // Guarantee only one copied version of each object.
  Object *cpy = (Object *)map_lookup(&copier->copy_map, obj);
  if (NULL != cpy) {
    return entity_object(cpy);
  }
  // Modules and Classes should not be copied.
  // These objects should be treated as effectively immutable (although as of
  // 2025-02-16, they are still mutable), allowing for pointer comparison and
  // shared use across threads.
  if (IS_CLASS(e, Class_Module) || IS_CLASS(e, Class_Class)) {
    map_insert(&copier->copy_map, obj, obj);
    return *e;
  }
  cpy = heap_new(copier->target, obj->_class);
  map_insert(&copier->copy_map, obj, cpy);

  if (NULL != obj->_class->_copy_fn) {
    obj->_class->_copy_fn(copier, obj, cpy);
  }

  KL_iter members = keyedlist_iter(&obj->_members);
  for (; kl_has(&members); kl_inc(&members)) {
    Entity member_cpy =
        entitycopier_copy(copier, &((Member *)kl_value(&members))->entity);
    object_set_member(copier->target, cpy, kl_key(&members), &member_cpy);
  }
  return entity_object(cpy);
}

Entity entity_copy(const Entity *e, Heap *target) {
  Entity copy;
  BULK_COPY(copier, target, { copy = entitycopier_copy(&copier, e); });
  return copy;
}

M_iter heapprofile_object_type_counts(const HeapProfile *const hp) {
  return map_iter((Map *)&hp->object_type_counts);
}

void heapprofile_delete(HeapProfile *hp) {
  map_finalize(&hp->object_type_counts);
  DEALLOC(hp);
}

HeapProfile *heap_create_profile(const Heap *const heap) {
  HeapProfile *hp = ALLOC2(HeapProfile);
  map_init_default(&hp->object_type_counts);
  const Set *nodes = mgraph_nodes(heap->mg);
  M_iter iter = set_iter((Set *)nodes);
  for (; has(&iter); inc(&iter)) {
    const Node *node = (Node *)value(&iter);
    const Object *obj = (const Object *)node_ptr(node);
    void *existing = map_lookup(&hp->object_type_counts, obj->_class);
    if (NULL == map_lookup(&hp->object_type_counts, obj->_class)) {
      map_insert(&hp->object_type_counts, obj->_class, (void *)(intptr_t)1);
    } else {
      map_insert(&hp->object_type_counts, obj->_class,
                 (void *)(intptr_t)(((int)(intptr_t)existing) + 1));
    }
  }
  return hp;
}