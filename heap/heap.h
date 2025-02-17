// heap.h
//
// Created on: May 28, 2020
//     Author: Jeff Manzione
//
// A memory-allocating heap for allocating new objects and deleting them once
// they are no longer referenced.
//
// NOTE: Heap is NOT threadsafe. Heap wraps MGraph, which is not threadsafe and
// provides no additional thread safety. To safely pass objects between a source
// and target heaps, the following must occur in the specified order:
//   1. The soure and target heaps must be locked.
//   2. The object from the source heap must be DEEPLY copied in the target heap
//      using entity_copy().
//   3. Locks are released.

#ifndef HEAP_HEAP_H_
#define HEAP_HEAP_H_

#include "debug/debug.h"
#include "entity/entity.h"
#include "entity/object.h"

typedef struct _Heap Heap;

typedef struct {
  MGraphConf mgraph_config;
  uint32_t max_object_count;
} HeapConf;

Heap *heap_create(HeapConf *config);
void heap_delete(Heap *heap);

DEB_FN(Object *, heap_new, Heap *heap, const Class *class);
#define heap_new(heap, class) CALL_FN(heap_new__, heap, class)

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

typedef struct _HeapProfile HeapProfile;

HeapProfile *heap_create_profile(const Heap *const heap);

M_iter heapprofile_object_type_counts(const HeapProfile *const hp);
void heapprofile_delete(HeapProfile *hp);

/**
 * @brief Struct for creating deep copies of multiple objects that should share
 * the same deep copies of objects.
 *
 */
struct _EntityCopier {
  Heap *target;
  /* Map of entities in source to their copies in target.*/
  Map copy_map;
};

typedef struct _EntityCopier EntityCopier;

/**
 * @brief Initializes an EntityCopier.
 *
 * @param copier The EntityCopier to initialize.
 * @param target The heap into which copies should be created.
 */
void entitycopier_init(EntityCopier *copier, Heap *target);

/**
 * @brief Finalizes the EntityCopier, destroying its copy state.
 *
 * @param copier
 */
void entitycopier_finalize(EntityCopier *copier);

/**
 * @brief Creates a copy of an Entity.
 *
 * @param copier The EntityCopier to create the copy.
 * @param e The entity to copy.
 * @return The newly-created entity.
 */
Entity entitycopier_copy(EntityCopier *copier, const Entity *e);

/**
 * @brief Creates a deep copy of an Entity in a Heap and returns it.
 *
 * This is only for use when copying one object. If multiple objects are being
 * copied and they should share the same underlying deep copies, then
 * EntityCopier should be used instead.
 *
 * @param e The entity to be copied.
 * @param heap The target heap for the copy.
 * @return The newly-created copy of e in heap.
 */
Entity entity_copy(const Entity *e, Heap *heap);

#define BULK_COPY(copier_var_name, target_heap, exp)                           \
  {                                                                            \
    EntityCopier copier_var_name;                                              \
    entitycopier_init(&copier_var_name, target_heap);                          \
    exp;                                                                       \
    entitycopier_finalize(&copier_var_name);                                   \
  }

#endif /* HEAP_HEAP_H_ */