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
  size_t dim1, dim2;
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
  const Entity *e_dim1 = tuple_get(tupl_args, 0);
  const Entity *e_dim2 = tuple_get(tupl_args, 1);

  if (!IS_INT(e_dim1) || !IS_INT(e_dim2)) {
    return raise_error(task, ctx, "Invalid input: expected (Int, Int).");
  }

  const size_t dim1 = pint(&e_dim1->pri);
  const size_t dim2 = pint(&e_dim2->pri);

  Int64Matrix *mat = NULL;
  obj->_internal_obj = mat = ALLOC2(Int64Matrix);
  mat->dim1 = dim1;
  mat->dim2 = dim2;

  mat->arr = Int64Array_create_sz(dim1 * dim2);
  memset(mat->arr->table, 0x0, sizeof(int64_t) * dim1 * dim2);

  return entity_object(obj);
}

Entity _Int64Matrix_shape(Task *task, Context *ctx, Object *obj, Entity *args) {
  const Int64Matrix *mat = (Int64Matrix *)obj->_internal_obj;
  Entity dim1 = entity_int(mat->dim1);
  Entity dim2 = entity_int(mat->dim2);
  Object *shape = tuple_create2(task->parent_process->heap, &dim1, &dim2);
  return entity_object(shape);
}

Entity _Int64Matrix_index(Task *task, Context *ctx, Object *obj, Entity *args) {
  const Int64Matrix *mat = (Int64Matrix *)obj->_internal_obj;
  if (IS_TUPLE(args)) {
    Tuple *targs = (Tuple *)args->obj->_internal_obj;
    if (tuple_size(targs) != 2) {
      return raise_error(task, ctx, "Expected exactly 2 args");
    }
    const Entity *first = tuple_get(targs, 0);
    const Entity *second = tuple_get(targs, 1);

    Tuple *dim1_indices = NULL, *dim2_indices = NULL;
    int dim1_index = -1, dim2_index = -1;
    _Range *dim1_range = NULL, *dim2_range = NULL;

    if (IS_TUPLE(first)) {
      dim1_indices = (Tuple *)first->obj->_internal_obj;
    } else if (IS_INT(first)) {
      dim1_index = pint(&first->pri);
      if (dim1_index < 0 || dim1_index >= mat->dim1) {
        return raise_error(task, ctx,
                           "Dim1 out of bounds: %d, must be in [0, %d)",
                           dim1_index, mat->dim1);
      }
    } else if (IS_CLASS(first, Class_Range)) {
      dim1_range = (_Range *)second->obj->_internal_obj;

    } else {
      return raise_error(task, ctx,
                         "Expected first arg to be an Int, Range, or tuple.");
    }

    if (IS_TUPLE(second)) {
      dim2_indices = (Tuple *)second->obj->_internal_obj;
    } else if (IS_INT(second)) {
      dim2_index = pint(&second->pri);
      if (dim2_index < 0 || dim2_index >= mat->dim2) {
        return raise_error(task, ctx,
                           "Dim2 out of bounds: %d, must be in [0, %d)",
                           dim2_index, mat->dim2);
      }
    } else if (IS_CLASS(second, Class_Range)) {
      dim2_range = (_Range *)second->obj->_internal_obj;
    } else {
      return raise_error(task, ctx,
                         "Expected second arg to be an Int, Range, or tuple.");
    }

  } else if (IS_INT(args)) {
    const int dim1_index = pint(&args->pri);
    if (dim1_index < 0 || dim1_index >= mat->dim1) {
      return raise_error(task, ctx,
                         "Index out of bounds: was %d, must be in [0,%d)",
                         dim1_index, mat->dim1);
    }
    Object *subarr = heap_new(task->parent_process->heap, Class_Int64Array);
    subarr->_internal_obj = Int64Array_create_copy(
        mat->arr->table + dim1_index * mat->dim2, mat->dim2);
    return entity_object(subarr);
  } else if (IS_CLASS(args, Class_Range)) {
  } else {
    return raise_error(task, ctx, "Expected tuple arg.");
  }

  return NONE_ENTITY;
}

void data_add_native(ModuleManager *mm, Module *data) {
  INSTALL_DATA_ARRAY(Int8Array, data);
  INSTALL_DATA_ARRAY(Int32Array, data);
  INSTALL_DATA_ARRAY(Int64Array, data);
  INSTALL_DATA_ARRAY(Float32Array, data);
  INSTALL_DATA_ARRAY(Float64Array, data);

  Class_Int64Matrix = native_class(data, intern("Int64Matrix"),
                                   _Int64Matrix_init, _Int64Array_delete);
  native_method(Class_Int64Matrix, CONSTRUCTOR_KEY, _Int64Matrix_constructor);
  native_method(Class_Int64Matrix, ARRAYLIKE_INDEX_KEY, _Int64Matrix_index);
  native_method(Class_Int64Matrix, intern("shape"), _Int64Matrix_shape);
}