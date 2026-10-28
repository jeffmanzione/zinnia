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

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_HEAP_HEAP_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_HEAP_HEAP_H_

#include "c-data-structures/maplike.h"
#include "zinnia/entity/object.h"
#include "zinnia/entity/value.h"
#include "zinnia/util/error.h"

DEFINE_MAPLIKE(ObjectTypeCountMap, Class *, int);
DEFINE_MAPLIKE(ObjectCopyMap, Object *, Object *);

typedef struct Heap_ Heap;

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
uint32_t heap_object_count_threshold_for_garbage_collection(
    const Heap *const heap);
void heap_set_object_count_threshold_for_garbage_collection(
    Heap *heap, uint32_t new_threshold);
void heap_make_root(Heap *heap, Object *obj);

void heap_inc_edge(Heap *heap, Object *parent, Object *child);
void heap_dec_edge(Heap *heap, Object *parent, Object *child);

void object_set_member(Heap *heap, Object *parent, const char key[],
                       const Value *child);
Value *object_set_member_obj(Heap *heap, Object *parent, const char key[],
                             const Object *child);

void array_add(Heap *heap, Object *array, const Value *child);
Value array_remove(Heap *heap, Object *array, int32_t index);
void array_set(Heap *heap, Object *array, int32_t index, const Value *child);
Object *array_create(Heap *heap);

void tuple_set(Heap *heap, Object *array, int32_t index, const Value *child);

Object *tuple_create_empty(Heap *heap, size_t size);
Object *tuple_create2(Heap *heap, Value *e1, Value *e2);
Object *tuple_create3(Heap *heap, Value *e1, Value *e2, Value *e3);
Object *tuple_create4(Heap *heap, Value *e1, Value *e2, Value *e3, Value *e4);
Object *tuple_create5(Heap *heap, Value *e1, Value *e2, Value *e3, Value *e4,
                      Value *e5);
Object *tuple_create6(Heap *heap, Value *e1, Value *e2, Value *e3, Value *e4,
                      Value *e5, Value *e6);
Object *tuple_create7(Heap *heap, Value *e1, Value *e2, Value *e3, Value *e4,
                      Value *e5, Value *e6, Value *e7);

typedef struct HeapProfile_ HeapProfile;

HeapProfile *heap_create_profile(const Heap *const heap);

ObjectTypeCountMapIterator heapprofile_object_type_counts(
    const HeapProfile *const hp);
void heapprofile_delete(HeapProfile *hp);

/**
 * @brief Struct for creating  deep copies of multiple objects that should share
 * the same deep copies of  objects.
 */
struct ValueCopier_ {
  Heap *target;
  /* Map of entities in source to their copies in target.*/
  ObjectCopyMap copy_map;
};

typedef struct ValueCopier_ ValueCopier;

/**
 * @brief Initializes an EntityCopier.
 *
 * @param copier The EntityCopier to initialize.
 * @param target The heap into which copies should be created.
 */
void valuecopier_init(ValueCopier *copier, Heap *target);

/**
 * @brief Finalizes the EntityCopier, destroying its copy state.
 *
 * @param copier
 */
void valuecopier_finalize(ValueCopier *copier);

/**
 * @brief Creates a copy of an Entity.
 *
 * @param copier The EntityCopier to create the copy.
 * @param e The entity to copy.
 * @return The newly-created entity.
 */
Value valuecopier_copy(ValueCopier *copier, const Value *e);

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
Value value_copy(const Value *e, Heap *heap);

#define BULK_COPY(copier_var_name, target_heap, exp) \
  {                                                  \
    ValueCopier copier_var_name;                     \
    valuecopier_init(&copier_var_name, target_heap); \
    exp;                                             \
    valuecopier_finalize(&copier_var_name);          \
  }

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_HEAP_HEAP_H_ */