// data.c
//
// Created on: July 7, 2023
//     Author: Jeff Manzione

#include "entity/native/data.h"

#include <stdint.h>

#include "entity/native/data_helper.h"

DEFINE_DATA_ARRAY(Int8Array, int8_t, entity_int);
DEFINE_DATA_ARRAY(Int32Array, int32_t, entity_int);
DEFINE_DATA_ARRAY(Int64Array, int64_t, entity_int);
DEFINE_DATA_ARRAY(Float32Array, float, entity_float);
DEFINE_DATA_ARRAY(Float64Array, double, entity_float);

static Class *Class_Int64Matrix = NULL;

typedef struct {
  Int64Array *arr;
  size_t width, length;
} Int64Matrix;

void _Int64Matrix_init(Object *obj) { obj->_internal_obj = NULL; }
void _Int64Matrix_delete(Object *obj) {
  Int64Array_delete(((Int64Matrix *)obj->_internal_obj)->arr);
  DEALLOC(obj->_internal_obj);
}

Entity _Int64Matrix_constructor(Task *task, Context *ctx, Object *obj,
                                Entity *args) {
  int initial_size;
  Int64Array *arr;
  if (!IS_TUPLE(args)) {
    return raise_error(task, ctx,
                       "Int64Matrix"
                       "requires a tuple argument.");
  }
  Tuple *tupl_args = (Tuple *)args->obj->_internal_obj;
  if (2 != tuple_size(tupl_args)) {
    return raise_error(task, ctx,
                       "Invalid number of arguments, expected 2, got %d",
                       tuple_size(tupl_args));
  }
  const Entity *e_width = tuple_get(tupl_args, 0);
  const Entity *e_length = tuple_get(tupl_args, 1);

  if (!IS_INT(e_width) || !IS_INT(e_length)) {
    return raise_error(task, ctx, "Invalid input: expected (Int, Int).");
  }

  const size_t width = pint(&e_width->pri);
  const size_t length = pint(&e_length->pri);

  Int64Matrix *mat = NULL;
  obj->_internal_obj = mat = ALLOC2(Int64Matrix);

  mat->arr = Int64Array_create_sz(width * length);
  memset(mat->arr->table, 0x0, sizeof(int64_t) * width * length);

  return entity_object(obj);
}

Entity _Int64Matrix_shape(Task *task, Context *ctx, Object *obj, Entity *args) {
  const Int64Matrix *mat = (Int64Matrix *)obj->_internal_obj;
  Entity width = entity_int(mat->width);
  Entity length = entity_int(mat->length);
  Object *shape = tuple_create2(task->parent_process->heap, &width, &length);
  return entity_object(shape);
}

void data_add_native(ModuleManager *mm, Module *data) {
  INSTALL_DATA_ARRAY(Int8Array, data);
  INSTALL_DATA_ARRAY(Int32Array, data);
  INSTALL_DATA_ARRAY(Int64Array, data);
  INSTALL_DATA_ARRAY(Float32Array, data);
  INSTALL_DATA_ARRAY(Float64Array, data);
}