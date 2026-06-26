// heap.h
//
// Created on: May 28, 2020
//     Author: Jeff Manzione

#include "zinnia/heap/heap.h"

#include <stdint.h>

#include "rzalloc/rzalloc.h"
#include "zinnia/alloc/alloc.h"
#include "zinnia/entity/array/array.h"
#include "zinnia/entity/class/classes_def.h"
#include "zinnia/entity/object.h"
#include "zinnia/entity/string/string.h"
#include "zinnia/entity/tuple/tuple.h"


IMPL_MAPLIKE(ObjectTypeCountMap, Class *, int);
IMPL_MAPLIKE(ObjectCopyMap, Object *, Object *);

uint32_t hash_class_(const Class *cl, uint32_t size) {
  return (uint32_t)(intptr_t)cl;
}

int32_t compare_classes_(const Class *cl1, uint32_t size1, const Class *cl2,
                         uint32_t size2) {
  return (intptr_t)cl1 - (intptr_t)cl2;
}

uint32_t hash_object_(const Object *ob, uint32_t size) {
  return (uint32_t)(intptr_t)ob;
}

int32_t compare_objects_(const Object *ob1, uint32_t size1, const Object *ob2,
                         uint32_t size2) {
  return (intptr_t)ob1 - (intptr_t)ob2;
}

struct Heap_ {
  HeapConf config;
  MGraph mg;
  RzallocArena object_arena;
  uint32_t object_count_threshold_for_garbage_collection;
};

struct HeapProfile_ {
  ObjectTypeCountMap object_type_counts;
};

Object *object_create_(Heap *heap, const Class *class);
void object_delete_(Object *object, Heap *heap);

Heap *heap_create(HeapConf *config) {
  ASSERT(config != NULL);
  Heap *heap = MNEW(Heap);
  config->mgraph_config.ctx = heap;
  heap->config = *config;
  heap->object_count_threshold_for_garbage_collection =
      config->max_object_count / 2;
  mgraph_init(&heap->mg, &heap->config.mgraph_config);
  arena_init(&heap->object_arena, sizeof(Object));
  return heap;
}

void heap_delete(Heap *heap) {
  ASSERT(heap != NULL);
  mgraph_finalize(&heap->mg);
  arena_clear(&heap->object_arena);
  RELEASE(heap);
}

uint32_t heap_collect_garbage(Heap *heap) {
  return mgraph_collect_garbage(&heap->mg);
}

uint32_t heap_object_count(const Heap *const heap) {
  ASSERT(heap != NULL);
  return mgraph_node_count(&heap->mg);
}

uint32_t heap_max_object_count(const Heap *const heap) {
  ASSERT(heap != NULL);
  return heap->config.max_object_count;
}

uint32_t heap_object_count_threshold_for_garbage_collection(
    const Heap *const heap) {
  ASSERT(heap != NULL);
  return heap->object_count_threshold_for_garbage_collection;
}

void heap_set_object_count_threshold_for_garbage_collection(
    Heap *heap, uint32_t new_threshold) {
  ASSERT(heap != NULL);
  heap->object_count_threshold_for_garbage_collection = new_threshold;
}

Object *heap_new(Heap *heap, const Class *class) {
  ASSERT(heap != NULL);
  ASSERT(class != NULL);
  Object *object = object_create_(heap, class);
  object->_node_ref =
      mgraph_insert(&heap->mg, object, (MGDeleter)object_delete_);
  return object;
}

void heap_make_root(Heap *heap, Object *obj) {
  mgraph_root(&heap->mg, (Node *)obj->_node_ref);
}

void object_set_member(Heap *heap, Object *parent, const char key[],
                       const Entity *child) {
  ASSERT(heap != NULL);
  ASSERT(parent != NULL);
  ASSERT(child != NULL);
  Entity *entry_pos;
  const bool was_insert =
      EntityMap_insert(&parent->_members, key, sizeof(char *), &entry_pos);
  ASSERT(entry_pos != NULL);
  const bool old_member_is_obj = !was_insert && OBJECT == etype(entry_pos);
  if (old_member_is_obj && (entry_pos->obj == child->obj)) {
    return;
  }
  if (old_member_is_obj) {
    heap_dec_edge(heap, parent, object_m(entry_pos));
  }
  if (OBJECT == etype(child)) {
    heap_inc_edge(heap, parent, object_m((Entity *)child));
  }
  (*entry_pos) = *child;
}

Entity *object_set_member_obj(Heap *heap, Object *parent, const char key[],
                              const Object *child) {
  ASSERT(heap != NULL);
  ASSERT(parent != NULL);
  ASSERT(child != NULL);
  Entity *entry_pos;

  const bool new_insert =
      EntityMap_insert(&parent->_members, key, sizeof(char *), &entry_pos);

  ASSERT(entry_pos != NULL);
  const bool old_member_is_obj = !new_insert && OBJECT == etype(entry_pos);
  if (old_member_is_obj && entry_pos->obj == child) {
    return entry_pos;
  }
  if (old_member_is_obj) {
    heap_dec_edge(heap, parent, object_m(entry_pos));
  }
  heap_inc_edge(heap, parent, (Object *)child);
  entry_pos->type = OBJECT;
  entry_pos->obj = (Object *)child;
  return entry_pos;
}

Object *object_create_(Heap *heap, const Class *class) {
  ASSERT(heap != NULL);
  Object *object = (Object *)arena_malloc(&heap->object_arena);
  object->_class = class;
  EntityMap_init(&object->_members, hash_interned_string,
                 compare_interned_strings);
  if (NULL != class->_init_fn) {
    class->_init_fn(object);
  }
  return object;
}

void print_object_summary_(Object *object) {
  ASSERT(object != NULL);
  if (object->_class == Class_Class) {
    printf("\tClass('%s')", object->_class_obj->_name);
  } else if (object->_class == Class_Module) {
    printf("\tModule('%s')", object->_module_obj->_name);
  } else if (object->_class == Class_Function) {
    const Function *func = object->_function_obj;
    if (NULL != func->_parent_class) {
      printf("\tMethod('%s.%s')", func->_parent_class->_name, func->_name);
    } else {
      printf("\tFunction('%s')", func->_name);
    }
  } else if (object->_class == Class_FunctionRef) {
    const _FunctionRef *fref = (_FunctionRef *)object->_internal_obj;
    printf("\tFunctionRef('%s.%s')", fref->obj->_class->_name,
           fref->func->_name);
  } else if (object->_class == Class_String) {
    printf("\tString('%.*s', %p)",
           (int)String_size(((String *)object->_internal_obj)),
           ((String *)object->_internal_obj)->table, object->_internal_obj);
  } else if (object->_class == Class_IString) {
    printf("\tIString('%.*s', %p)", ((IString *)object->_internal_obj)->len,
           ((IString *)object->_internal_obj)->str, object->_internal_obj);
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
  const NodeSet *nodes = mgraph_nodes(&heap->mg);
  NodeSetIterator iter;
  NodeSet_iterator(&iter, nodes);
  for (; NodeSet_has_next(&iter); NodeSet_next(&iter)) {
    const Node *node = (Node *)NodeSet_value(&iter);
    Object *obj = (Object *)node_ptr(node);
    print_object_summary_(obj);
  }
}

void object_delete_(Object *object, Heap *heap) {
  ASSERT(heap != NULL);
  ASSERT(object != NULL);
  // print_object_summary_(object);
  if (NULL != object->_class->_delete_fn) {
    object->_class->_delete_fn(object);
  }
  EntityMap_finalize(&object->_members);
  arena_free(&heap->object_arena, object);
}

void heap_inc_edge(Heap *heap, Object *parent, Object *child) {
  ASSERT(heap != NULL);
  ASSERT(parent != NULL);
  ASSERT(child != NULL);
  mgraph_inc(&heap->mg, (Node *)parent->_node_ref, (Node *)child->_node_ref);
}

void heap_dec_edge(Heap *heap, Object *parent, Object *child) {
  ASSERT(heap != NULL);
  ASSERT(parent != NULL);
  ASSERT(child != NULL);
  mgraph_dec(&heap->mg, (Node *)parent->_node_ref, (Node *)child->_node_ref);
}

void array_add(Heap *heap, Object *array, const Entity *child) {
  ASSERT(heap != NULL);
  ASSERT(array != NULL);
  ASSERT(child != NULL);
  Entity *e = Array_push_back_ref((Array *)array->_internal_obj);
  *e = *child;
  if (OBJECT != child->type) {
    return;
  }
  mgraph_inc(&heap->mg, (Node *)array->_node_ref,
             (Node *)child->obj->_node_ref);
}

Entity array_remove(Heap *heap, Object *array, int32_t index) {
  ASSERT(heap != NULL);
  ASSERT(array != NULL);
  ASSERT(index >= 0);
  Entity e = Array_remove_unchecked((Array *)array->_internal_obj, index);
  if (OBJECT == e.type) {
    heap_dec_edge(heap, array, e.obj);
  }
  return e;
}

void array_set(Heap *heap, Object *array, int32_t index, const Entity *child) {
  ASSERT(heap != NULL);
  ASSERT(array != NULL);
  ASSERT(child != NULL);
  ASSERT(index >= 0);
  Entity *e = Array_set_ref_unchecked((Array *)array->_internal_obj, index);
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
  ASSERT(heap != NULL);
  ASSERT(tuple != NULL);
  ASSERT(child != NULL);
  ASSERT(index >= 0);
  ASSERT(index < tuple_size((Tuple *)tuple->_internal_obj));
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
  ASSERT(copier != NULL);
  ASSERT(target != NULL);
  copier->target = target;
  DEBUGF("=)");
  ObjectCopyMap_init(&copier->copy_map, hash_object_, compare_objects_);
}

void entitycopier_finalize(EntityCopier *copier) {
  ASSERT(copier != NULL);
  ObjectCopyMap_finalize(&copier->copy_map);
}

Entity entitycopier_copy(EntityCopier *copier, const Entity *e) {
  ASSERT(copier != NULL);
  ASSERT(e != NULL);
  ASSERT(e != NULL);
  switch (e->type) {
    case NONE:
    case PRIMITIVE:
      return *e;
    default:
      ASSERT(OBJECT == e->type);
  }
  Object *obj = e->obj;
  // Guarantee only one copied version of each object.
  Object *cpy =
      ObjectCopyMap_find(&copier->copy_map, obj, sizeof(Object *), NULL);
  if (NULL != cpy) {
    return entity_object(cpy);
  }
  // Modules and Classes should not be copied.
  // These objects should be treated as effectively immutable (although as of
  // 2025-02-16, they are still mutable), allowing for pointer comparison and
  // shared use across threads.
  if (IS_CLASS(e, Class_Module) || IS_CLASS(e, Class_Class)) {
    ObjectCopyMap_insert(&copier->copy_map, obj, sizeof(Object *), obj);
    return *e;
  }
  cpy = heap_new(copier->target, obj->_class);
  ObjectCopyMap_insert(&copier->copy_map, obj, sizeof(Object *), cpy);

  if (NULL != obj->_class->_copy_fn) {
    obj->_class->_copy_fn(copier, obj, cpy);
  }

  EntityMapIterator members;
  EntityMap_iterator(&members, &obj->_members);
  for (; EntityMap_has_entry(&members); EntityMap_next_entry(&members)) {
    Entity member_cpy = entitycopier_copy(copier, EntityMap_value(&members));
    object_set_member(copier->target, cpy, EntityMap_key(&members),
                      &member_cpy);
  }
  return entity_object(cpy);
}

Entity entity_copy(const Entity *e, Heap *target) {
  Entity copy;
  BULK_COPY(copier, target, { copy = entitycopier_copy(&copier, e); });
  return copy;
}

ObjectTypeCountMapIterator heapprofile_object_type_counts(
    const HeapProfile *const hp) {
  ObjectTypeCountMapIterator it;
  ObjectTypeCountMap_iterator(&it, &hp->object_type_counts);
  return it;
}

void heapprofile_delete(HeapProfile *hp) {
  ObjectTypeCountMap_finalize(&hp->object_type_counts);
  RELEASE(hp);
}

HeapProfile *heap_create_profile(const Heap *const heap) {
  HeapProfile *hp = MNEW(HeapProfile);
  ObjectTypeCountMap_init(&hp->object_type_counts, hash_class_,
                          compare_classes_);
  const NodeSet *nodes = mgraph_nodes(&heap->mg);
  NodeSetIterator iter;
  NodeSet_iterator(&iter, nodes);
  for (; NodeSet_has_next(&iter); NodeSet_next(&iter)) {
    const Node *node = *NodeSet_value(&iter);
    const Object *obj = (const Object *)node_ptr(node);
    const int existing = ObjectTypeCountMap_find(
        &hp->object_type_counts, obj->_class, sizeof(Class *), 0);
    ObjectTypeCountMap_insert(&hp->object_type_counts, obj->_class,
                              sizeof(Class *), existing + 1);
  }
  return hp;
}