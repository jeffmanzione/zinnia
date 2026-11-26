// heap.h
//
// Created on: May 28, 2020
//     Author: Jeff Manzione
//
// A memory-allocating heap for allocating new objects and deleting them once
// they are no longer referenced.
//
// Garbage is found by marking: what can be reached from the roots, from the
// objects passed to heap_mark() and from what they refer to (their members,
// what's in an Array or a Tuple, and the edges heap_inc_edge() adds) is kept,
// and the rest is deleted.
//
// NOTE: Heap is NOT threadsafe. To safely pass objects between a source
// and target heaps, the following must occur in the specified order:
//   1. The soure and target heaps must be locked.
//   2. The object from the source heap must be DEEPLY copied in the target heap
//      using value_copy().
//   3. Locks are released.

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_HEAP_HEAP_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_HEAP_HEAP_H_

#include "c-data-structures/maplike.h"
#include "zinnia/util/error.h"
#include "zinnia/value/object.h"
#include "zinnia/value/string/string.h"
#include "zinnia/value/value.h"

DEFINE_MAPLIKE(ObjectTypeCountMap, Class *, int);
DEFINE_MAPLIKE(ObjectCopyMap, Object *, Object *);

typedef struct Heap_ Heap;

typedef struct {
  uint32_t max_object_count;
} HeapConf;

Heap *heap_create(HeapConf *config);
void heap_delete(Heap *heap);

// The FunctionRef binding [func] to [obj] made earlier in [heap], if it's still
// alive, or NULL. A FunctionRef of a method never changes, so one can be
// reused instead of making a new one for every lookup.
Object *heap_bound_method(Heap *heap, const Object *obj, const void *func);
void heap_set_bound_method(Heap *heap, const Object *obj, const void *func,
                           Object *fn_ref);
// Forgets [fn_ref], which is being collected, if it's the one remembered.
void heap_forget_bound_method(Heap *heap, const Object *obj, const void *func,
                              const Object *fn_ref);

Object *heap_new(Heap *heap, const Class *class);
// Room for the String of a String object in [heap], which frees it when it
// deletes the object. See string_init__().
String *heap_string_storage(Heap *heap);

// Keeps [obj] through the next heap_collect_garbage(), with what it refers to.
// Objects in other heaps are left alone.
void heap_mark(Heap *heap, Object *obj);
// Deletes the objects that can't be reached from the roots or from what was
// passed to heap_mark() since the last collection. Returns how many it deleted.
uint32_t heap_collect_garbage(Heap *heap);
void heap_print_debug_summary(Heap *heap);
uint32_t heap_object_count(const Heap *const heap);
uint32_t heap_max_object_count(const Heap *const heap);
uint32_t heap_object_count_threshold_for_garbage_collection(
    const Heap *const heap);
void heap_set_object_count_threshold_for_garbage_collection(
    Heap *heap, uint32_t new_threshold);
void heap_make_root(Heap *heap, Object *obj);

// A reference from [parent] to [child] the collector can't otherwise see,
// because it's kept in C rather than in a member, an Array or a Tuple. Each
// heap_inc_edge() is undone by one heap_dec_edge(). Edges between heaps are
// left out.
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
  /* Map of values in source to their copies in target.*/
  ObjectCopyMap copy_map;
};

typedef struct ValueCopier_ ValueCopier;

/**
 * @brief Initializes a ValueCopier.
 *
 * @param copier The ValueCopier to initialize.
 * @param target The heap into which copies should be created.
 */
void valuecopier_init(ValueCopier *copier, Heap *target);

/**
 * @brief Finalizes the ValueCopier, destroying its copy state.
 *
 * @param copier
 */
void valuecopier_finalize(ValueCopier *copier);

/**
 * @brief Creates a copy of a Value.
 *
 * @param copier The ValueCopier to create the copy.
 * @param e The value to copy.
 * @return The newly-created value.
 */
Value valuecopier_copy(ValueCopier *copier, const Value *e);

/**
 * @brief Creates a deep copy of a Value in a Heap and returns it.
 *
 * This is only for use when copying one object. If multiple objects are being
 * copied and they should share the same underlying deep copies, then
 * ValueCopier should be used instead.
 *
 * @param e The value to be copied.
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