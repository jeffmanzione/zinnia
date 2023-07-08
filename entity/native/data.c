// data.c
//
// Created on: July 7, 2023
//     Author: Jeff Manzione

#include "entity/native/data.h"

#include <stdint.h>

#include "alloc/arena/intern.h"
#include "debug/debug.h"
#include "entity/array/array.h"
#include "entity/class/classes_def.h"
#include "entity/entity.h"
#include "entity/native/builtin.h"
#include "entity/native/error.h"
#include "entity/native/native.h"
#include "entity/object.h"
#include "entity/tuple/tuple.h"
#include "struct/arraylike.h"
#include "vm/intern.h"
#include "vm/process/context.h"
#include "vm/process/processes.h"
#include "vm/process/task.h"

static Class *Class_Int64Array = NULL;

#define INT64ARRAY_INITIAL_SIZE 16

DEFINE_ARRAYLIKE(Int64Array, int64_t);
IMPL_ARRAYLIKE(Int64Array, int64_t);

void _Int64Array_init(Object *obj) { obj->_internal_obj = NULL; }
void _Int64Array_delete(Object *obj) { Int64Array_delete(obj->_internal_obj); }

int64_t _to_int64(const Entity *e) {
  switch (e->type) {
  case NONE:
    return 0L;
  case OBJECT:
    return (int64_t)e->obj;
  case PRIMITIVE:
    switch (ptype(&e->pri)) {
    case PRIMITIVE_CHAR:
      return (int64_t)pchar(&e->pri);
    case PRIMITIVE_INT:
      return pint(&e->pri);
    case PRIMITIVE_FLOAT:
      return (int64_t)pfloat(&e->pri);
    default:
      FATALF("Unknown primitive type.");
    }
  }
  FATALF("Unknown type.");
  return 0;
}

Entity _Int64Array_constructor(Task *task, Context *ctx, Object *obj,
                               Entity *args) {
  int initial_size;
  Int64Array *int64arr;
  if (NULL == args || IS_NONE(args)) {
    initial_size = INT64ARRAY_INITIAL_SIZE;
  } else if (IS_INT(args)) {
    initial_size = pint(&args->pri);
  } else if (IS_CLASS(args, Class_Array)) {
    Array *arr = (Array *)args->obj->_internal_obj;
    initial_size = Array_size(arr);
  } else {
    return raise_error(
        task, ctx, "Error argument must either not be present or be an Int.");
  }
  obj->_internal_obj = int64arr = Int64Array_create_sz(initial_size);

  if (IS_INT(args)) {
    Int64Array_set(int64arr, initial_size - 1, 0);
  } else if (IS_CLASS(args, Class_Array)) {
    Array *arr = (Array *)args->obj->_internal_obj;
    for (int i = 0; i < Array_size(arr); ++i) {
      Entity *e = Array_get_ref(arr, i);
      int64_t ie = _to_int64(e);
      Int64Array_set(int64arr, i, ie);
    }
  } else {
    memset(int64arr->table, 0x0, sizeof(int64_t) * initial_size);
  }
  return entity_object(obj);
}

Entity _Int64Array_len(Task *task, Context *ctx, Object *obj, Entity *args) {
  return entity_int(Int64Array_size((Int64Array *)obj->_internal_obj));
}

Entity _Int64Array_index_range(Task *task, Context *ctx, Int64Array *self,
                               _Range *range) {
  Object *obj = heap_new(task->parent_process->heap, Class_Int64Array);
  if (range->inc == 1) {
    size_t new_len = range->end - range->start;
    obj->_internal_obj =
        Int64Array_create_copy(self->table + range->start, new_len);
  } else {
    size_t new_len = (range->end - range->start) / range->inc;
    Int64Array *arr = Int64Array_create_sz(new_len);
    for (int i = range->start, j = 0; i < range->end; i += range->inc, ++j) {
      Int64Array_set(arr, j, Int64Array_get(self, i));
    }
    obj->_internal_obj = arr;
  }
  return entity_object(obj);
}

Entity _Int64Array_index(Task *task, Context *ctx, Object *obj, Entity *args) {
  ASSERT(NOT_NULL(args));
  Int64Array *self = (Int64Array *)obj->_internal_obj;

  if (IS_CLASS(args, Class_Range)) {
    _Range *range = (_Range *)args->obj->_internal_obj;
    return _Int64Array_index_range(task, ctx, self, range);
  }

  if (PRIMITIVE != args->type || PRIMITIVE_INT != ptype(&args->pri)) {
    return raise_error(task, ctx, "Bad Int64Array index");
  }

  int32_t index = pint(&args->pri);

  if (index < 0 || index >= Int64Array_size(self)) {
    return raise_error(task, ctx, "Index out of bounds");
  }

  return entity_int(Int64Array_get(self, index));
}

Entity _Int64Array_set(Task *task, Context *ctx, Object *obj, Entity *args) {
  Int64Array *arr = (Int64Array *)obj->_internal_obj;
  if (!IS_TUPLE(args)) {
    return raise_error(task, ctx, "Expected tuple input.");
  }
  Tuple *tupl_args = (Tuple *)args->obj->_internal_obj;
  if (2 != tuple_size(tupl_args)) {
    return raise_error(task, ctx,
                       "Ïnvalid number of arguments, expected 2, got %d",
                       tuple_size(tupl_args));
  }
  const Entity *index = tuple_get(tupl_args, 0);
  const Entity *val = tuple_get(tupl_args, 1);

  if (NULL == index || PRIMITIVE != index->type ||
      PRIMITIVE_INT != ptype(&index->pri)) {
    return raise_error(task, ctx,
                       "Cannot index a string with something not an int.");
  }
  int64_t indexi = pint(&index->pri);
  if (indexi < 0 || indexi >= Int64Array_size(arr)) {
    return raise_error(task, ctx, "Index out of bounds");
  }

  int64_t ival = _to_int64(val);
  Int64Array_set(arr, pint(&index->pri), ival);

  return NONE_ENTITY;
}

Entity _Int64Array_to_arr(Task *task, Context *ctx, Object *obj, Entity *args) {
  ASSERT(NOT_NULL(args));
  Int64Array *self = (Int64Array *)obj->_internal_obj;

  Object *arre = array_create(task->parent_process->heap);
  Array *arr = (Array *)arre->_internal_obj;

  for (int i = 0; i < Int64Array_size(self); ++i) {
    *Array_set_ref(arr, i) = entity_int(Int64Array_get(self, i));
  }
  return entity_object(arre);
}

void data_add_native(ModuleManager *mm, Module *data) {
  Class_Int64Array = native_class(data, intern("Int64Array"), _Int64Array_init,
                                  _Int64Array_delete);
  native_method(Class_Int64Array, CONSTRUCTOR_KEY, _Int64Array_constructor);
  native_method(Class_Int64Array, intern("len"), _Int64Array_len);
  native_method(Class_Int64Array, ARRAYLIKE_INDEX_KEY, _Int64Array_index);
  native_method(Class_Int64Array, ARRAYLIKE_SET_KEY, _Int64Array_set);
  native_method(Class_Int64Array, intern("to_arr"), _Int64Array_to_arr);
}