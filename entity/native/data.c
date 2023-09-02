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

Int64Matrix *_Int64Matrix_create(int dim1, int dim2, bool clear) {
  Int64Matrix *mat = ALLOC2(Int64Matrix);
  mat->dim1 = dim1;
  mat->dim2 = dim2;
  mat->arr = Int64Array_create_sz(dim1 * dim2);
  if (clear) {
    memset(mat->arr->table, 0x0, sizeof(int64_t) * dim1 * dim2);
  }
  return mat;
}

Entity _Int64Matrix_constructor(Task *task, Context *ctx, Object *obj,
                                Entity *args) {
  if (IS_TUPLE(args)) {
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

    obj->_internal_obj = _Int64Matrix_create(dim1, dim2, /*clear=*/true);
  } else if (IS_ARRAY(args)) {
    Array *arr = (Array *)args->obj->_internal_obj;
    const size_t dim1 = Array_size(arr);
    int max_dim2 = 0;
    for (int i = 0; i < Array_size(arr); ++i) {
      Entity *e = Array_get_ref(arr, i);
      if (IS_ARRAY(e)) {
        max_dim2 = max(max_dim2, Array_size((Array *)e->obj->_internal_obj));
      } else if (IS_CLASS(e, Class_Int64Array)) {
        max_dim2 =
            max(max_dim2, Int64Array_size((Int64Array *)e->obj->_internal_obj));
      } else {
        return raise_error(task, ctx,
                           "Invalid member of input array at index %d", i);
      }
    }
    Int64Matrix *mat;
    obj->_internal_obj = mat =
        _Int64Matrix_create(dim1, max_dim2, /*clear=*/true);

    for (int i = 0; i < Array_size(arr); ++i) {
      Entity *e = Array_get_ref(arr, i);
      if (IS_ARRAY(e)) {
        Array *arri = (Array *)e->obj->_internal_obj;
        for (int j = 0; j < Array_size(arri); ++j) {
          int64_t val = pint(&Array_get_ref(arri, j)->pri);
          mat->arr->table[i * mat->dim2 + j] = val;
        }
      } else /*if (IS_CLASS(e, Class_Int64Array))*/ {
        const Int64Array *arri = (Int64Array *)e->obj->_internal_obj;
        memmove(mat->arr->table + i * mat->dim2, arri->table,
                sizeof(int64_t) * mat->dim2);
      }
    }
  }
  return entity_object(obj);
}

Entity _Int64Matrix_shape(Task *task, Context *ctx, Object *obj, Entity *args) {
  const Int64Matrix *mat = (Int64Matrix *)obj->_internal_obj;
  Entity dim1 = entity_int(mat->dim1);
  Entity dim2 = entity_int(mat->dim2);
  Object *shape = tuple_create2(task->parent_process->heap, &dim1, &dim2);
  return entity_object(shape);
}

inline size_t _get_dim(const Int64Matrix *mat, int dim) {
  return dim == 1 ? mat->dim1 : mat->dim2;
}

inline int _first_bad_index(const Int64Matrix *mat, int dim,
                            const Tuple *dim_indices) {
  for (int i = 0; i < tuple_size(dim_indices); ++i) {
    const Entity *e = tuple_get(dim_indices, i);
    if (!IS_INT(e) || pint(&e->pri) < 0 ||
        pint(&e->pri) >= _get_dim(mat, dim)) {
      return i;
    }
  }
  return -1;
}

inline bool _range_is_valid(const Int64Matrix *mat, int dim,
                            const _Range *range) {
  return range->start >= 0 && range->end <= _get_dim(mat, dim);
}

inline bool _index_is_valid(const Int64Matrix *mat, int dim, int index) {
  return index >= 0 && index < _get_dim(mat, dim);
}

inline Entity _compute_indices(Task *task, Context *ctx, const Entity *arg,
                               const Int64Matrix *mat, int dim, int *dim_index,
                               Tuple **dim_indices, _Range **dim_range) {
  if (IS_INT(arg)) {
    *dim_index = pint(&arg->pri);
    if (!_index_is_valid(mat, dim, *dim_index)) {
      return raise_error(task, ctx,
                         "Dim%d out of bounds: %d, must be in [0, %d)", dim,
                         *dim_index, _get_dim(mat, dim));
    }
  } else if (IS_TUPLE(arg)) {
    *dim_indices = (Tuple *)arg->obj->_internal_obj;
    int bad_index = -1;
    if (bad_index = _first_bad_index(mat, dim, *dim_indices) >= 0) {
      return raise_error(task, ctx, "Invalid dim%d index at %d", dim,
                         bad_index);
    }
  } else if (IS_CLASS(arg, Class_Range)) {
    *dim_range = (_Range *)arg->obj->_internal_obj;
    if (!_range_is_valid(mat, dim, *dim_range)) {
      return raise_error(task, ctx,
                         "Dim%d out of bounds: (%d:%d:%d), must be in [0, %d)",
                         dim, (*dim_range)->start, (*dim_range)->end,
                         (*dim_range)->inc, _get_dim(mat, dim));
    }
  } else {
    return raise_error(task, ctx,
                       "Expected dim%d arg to be an Int, Range, or tuple", dim);
  }
  return NONE_ENTITY;
}

inline int64_t _get_value(const Int64Matrix *mat, int dim1_index,
                          int dim2_index) {
  return Int64Array_get(mat->arr, mat->dim2 * dim1_index + dim2_index);
}

inline size_t _range_size(const _Range *range) {
  return ceil(1.0 * (range->end - range->start) / range->inc);
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

    int dim1_index = -1, dim2_index = -1;
    Tuple *dim1_indices = NULL, *dim2_indices = NULL;
    _Range *dim1_range = NULL, *dim2_range = NULL;

    const Entity dim1_e = _compute_indices(
        task, ctx, first, mat, 1, &dim1_index, &dim1_indices, &dim1_range);
    if (!IS_NONE(&dim1_e)) {
      return dim1_e;
    }

    const Entity dim2_e = _compute_indices(
        task, ctx, second, mat, 2, &dim2_index, &dim2_indices, &dim2_range);
    if (!IS_NONE(&dim2_e)) {
      return dim2_e;
    }

    if (dim1_index >= 0) {
      if (dim2_index >= 0) {
        return entity_int(_get_value(mat, dim1_index, dim2_index));
      } else if (NULL != dim2_indices) {
        Int64Array *new_arr;
        Object *new_obj =
            heap_new(task->parent_process->heap, Class_Int64Array);
        new_obj->_internal_obj = new_arr =
            Int64Array_create_sz(tuple_size(dim2_indices));
        for (int i = 0; i < tuple_size(dim2_indices); ++i) {
          Int64Array_set(new_arr, i,
                         _get_value(mat, dim1_index,
                                    pint(&tuple_get(dim2_indices, i)->pri)));
        }
        return entity_object(new_obj);
      } else /* (NULL != dim2_range) */ {
        Int64Array *new_arr;
        Object *new_obj =
            heap_new(task->parent_process->heap, Class_Int64Array);
        new_obj->_internal_obj = new_arr =
            Int64Array_create_sz(_range_size(dim2_range));
        for (int i = dim2_range->start, j = 0;
             dim2_range->inc > 0 ? i < dim2_range->end : i > dim2_range->end;
             i += dim2_range->inc, ++j) {
          Int64Array_set(new_arr, j, _get_value(mat, dim1_index, i));
        }
        return entity_object(new_obj);
      }
    } else if (NULL != dim1_indices) {
      if (dim2_index >= 0) {
        Int64Array *new_arr;
        Object *new_obj =
            heap_new(task->parent_process->heap, Class_Int64Array);
        new_obj->_internal_obj = new_arr =
            Int64Array_create_sz(tuple_size(dim1_indices));
        for (int i = 0; i < tuple_size(dim1_indices); ++i) {
          Int64Array_set(new_arr, i,
                         _get_value(mat, pint(&tuple_get(dim1_indices, i)->pri),
                                    dim2_index));
        }
        return entity_object(new_obj);
      } else if (NULL != dim2_indices) {
        Int64Matrix *new_mat;
        Object *new_obj =
            heap_new(task->parent_process->heap, Class_Int64Matrix);
        new_obj->_internal_obj = new_mat =
            _Int64Matrix_create(tuple_size(dim1_indices),
                                tuple_size(dim2_indices), /*clear=*/false);
        for (int i = 0; i < tuple_size(dim1_indices); ++i) {
          for (int j = 0; j < tuple_size(dim2_indices); ++j) {
            Int64Array_set(new_mat->arr, i * new_mat->dim2 + j,
                           _get_value(mat,
                                      pint(&tuple_get(dim1_indices, i)->pri),
                                      pint(&tuple_get(dim2_indices, j)->pri)));
          }
        }
        return entity_object(new_obj);
      } else /* (NULL != dim2_range) */ {
        Int64Matrix *new_mat;
        Object *new_obj =
            heap_new(task->parent_process->heap, Class_Int64Matrix);
        new_obj->_internal_obj = new_mat = _Int64Matrix_create(
            tuple_size(dim1_indices), _range_size(dim2_range), /*clear=*/false);
        for (int i = 0; i < tuple_size(dim1_indices); ++i) {
          for (int i2 = dim2_range->start, j = 0;
               dim2_range->inc > 0 ? i2 < dim2_range->end
                                   : i2 > dim2_range->end;
               i2 += dim2_range->inc, ++j) {
            Int64Array_set(
                new_mat->arr, i * new_mat->dim2 + j,
                _get_value(mat, pint(&tuple_get(dim1_indices, i)->pri), i2));
          }
        }
        return entity_object(new_obj);
      }
    } else /* if (NULL != dim1_range) */ {
      if (dim2_index >= 0) {
        Int64Array *new_arr;
        Object *new_obj =
            heap_new(task->parent_process->heap, Class_Int64Array);
        new_obj->_internal_obj = new_arr =
            Int64Array_create_sz(_range_size(dim1_range));
        for (int i = dim1_range->start, j = 0;
             dim1_range->inc > 0 ? i < dim1_range->end : i > dim1_range->end;
             i += dim1_range->inc, ++j) {
          Int64Array_set(new_arr, j, _get_value(mat, i, dim2_index));
        }
        return entity_object(new_obj);
      } else if (NULL != dim2_indices) {
        Int64Matrix *new_mat;
        Object *new_obj =
            heap_new(task->parent_process->heap, Class_Int64Matrix);
        new_obj->_internal_obj = new_mat = _Int64Matrix_create(
            _range_size(dim1_range), tuple_size(dim2_indices), /*clear=*/false);
        for (int i = 0; i < tuple_size(dim2_indices); ++i) {
          for (int i2 = dim1_range->start, j = 0;
               dim1_range->inc > 0 ? i2 < dim1_range->end
                                   : i2 > dim1_range->end;
               i2 += dim1_range->inc, ++j) {
            Int64Array_set(
                new_mat->arr, j * new_mat->dim2 + i,
                _get_value(mat, i2, pint(&tuple_get(dim2_indices, i)->pri)));
          }
        }
        return entity_object(new_obj);
      } else /* (NULL != dim2_range) */ {
        Int64Matrix *new_mat;
        Object *new_obj =
            heap_new(task->parent_process->heap, Class_Int64Matrix);
        new_obj->_internal_obj = new_mat = _Int64Matrix_create(
            _range_size(dim1_range), _range_size(dim2_range), /*clear=*/false);
        for (int i = dim1_range->start, j = 0;
             dim1_range->inc > 0 ? i < dim1_range->end : i > dim1_range->end;
             i += dim1_range->inc, ++j) {
          for (int k = dim2_range->start, l = 0;
               dim2_range->inc > 0 ? k < dim2_range->end : k > dim2_range->end;
               k += dim2_range->inc, ++l) {
            Int64Array_set(new_mat->arr, j * new_mat->dim2 + l,
                           _get_value(mat, i, k));
          }
        }
        return entity_object(new_obj);
      }
    }
  } else if (IS_INT(args)) {
    const int dim1_index = (int)pint(&args->pri);
    if (!_index_is_valid(mat, 1, dim1_index)) {
      return raise_error(task, ctx,
                         "Index out of bounds: was %d, must be in [0,%d)",
                         dim1_index, mat->dim1);
    }
    Object *subarr = heap_new(task->parent_process->heap, Class_Int64Array);
    subarr->_internal_obj = Int64Array_create_copy(
        mat->arr->table + dim1_index * mat->dim2, mat->dim2);
    return entity_object(subarr);
  } else if (IS_CLASS(args, Class_Range)) {
    const _Range *range = (_Range *)args->obj->_internal_obj;
    if (!_range_is_valid(mat, 1, range)) {
      return raise_error(task, ctx,
                         "Range out of bounds: (%d:%d:%d) vs size=%d",
                         range->start, range->end, range->inc, mat->dim1);
    }
    const size_t new_dim1 = _range_size(range);

    Object *submat_o = heap_new(task->parent_process->heap, Class_Int64Matrix);
    Int64Matrix *submat;
    submat_o->_internal_obj = submat =
        _Int64Matrix_create(new_dim1, mat->dim2, /*clear=*/false);
    for (int i = range->start, j = 0;
         range->inc > 0 ? i < range->end : i > range->end;
         i += range->inc, ++j) {
      memmove(submat->arr->table + j * mat->dim2,
              mat->arr->table + i * mat->dim2, sizeof(int64_t) * mat->dim2);
    }
    return entity_object(submat_o);
  } else {
    return raise_error(task, ctx, "Expected tuple arg.");
  }
  return NONE_ENTITY;
}

// inline int _compute_length(Task *task, Context *ctx, const Entity *dim,
//                            Entity *error) {
//   if (IS_INT(dim)) {
//     return pint(&dim->pri);
//   } else if (IS_TUPLE(dim)) {
//     return tuple_size((Tuple *)dim->obj->_internal_obj);
//   } else if (IS_CLASS(dim, Class_Range)) {
//     return _range_size((_Range *)dim->obj->_internal_obj);
//   } else {
//     *error = raise_error(task, ctx, "Invalid dimension type");
//   }
//   return -1;
// }

// inline void _compute_shape(Task *task, Context *ctx, const Entity *dim1,
//                            const Entity *dim2, int *dim1_len, int *dim2_len)
//                            {
//   Entity error = NONE_ENTITY;
//   *dim1_len = _compute_length(task, ctx, dim1, &error);
//   if (!IS_NONE(&error)) {
//     return;
//   }
//   *dim2_len = _compute_length(task, ctx, dim2, &error);
// }

inline void _add_indices(Task *task, Context *ctx, const Entity *arg,
                         Int64Matrix *mat, int dim, AList *indices,
                         Entity *error) {
  int index = -1;
  Tuple *tuple = NULL;
  _Range *range = NULL;
  Entity error = _compute_indices(task, ctx, arg, mat, dim, &tuple, &range);

  if (index >= 0) {
    *(int *)alist_add(indices) = index;
  } else if (NULL != tuple) {
    for (int i = 0; i < tuple_size(tuple); ++i) {
      *(int *)alist_add(indices) = (int)pint(&tuple_get(tuple, i)->pri);
    }
  } else if (NULL != range) {
    for (int i = range->start, j = 0;
         range->inc > 0 ? i < range->end : i > range->end;
         i += range->inc, ++j) {
      *(int *)alist_add(indices) = i;
    }
  } else {
    *error = raise_error(task, ctx, "Invalid dimension type");
  }
}

Entity _Int64Matrix_set(Task *task, Context *ctx, Object *obj, Entity *args) {
  const Int64Matrix *mat = (Int64Matrix *)obj->_internal_obj;

  if (!IS_TUPLE(args)) {
    return raise_error(task, ctx, "Expected tuple input");
  }
  Tuple *targs = (Tuple *)args->obj->_internal_obj;
  if (tuple_size(targs) != 2) {
    return raise_error(task, ctx, "Expected exactly 2 args");
  }
  const Entity *lhs = tuple_get(targs, 0);
  const Entity *rhs = tuple_get(targs, 1);

  AList dim1_indices, dim2_indices;
  alist_init(&dim1_indices, int, 8);
  alist_init(&dim2_indices, int, 8);

  Entity error = NONE_ENTITY;
  _add_indices(task, ctx, first, mat, 1, &dim1_indices, &error);
  _add_indices(task, ctx, second, mat, 2, &dim2_indices, &error);

  for (int i = 0; i < alist_len(&dim1_indices); ++i) {
    for (int j = 0; j < alist_len(&dim2_indices); ++j) {
    }
  }

  alist_finalize(&dim1_indices);
  alist_finalize(&dim2_indices);

  return NONE_ENTITY;
}

void data_add_native(ModuleManager *mm, Module *data) {
  INSTALL_DATA_ARRAY(Int8Array, data);
  INSTALL_DATA_ARRAY(Int32Array, data);
  INSTALL_DATA_ARRAY(Int64Array, data);
  INSTALL_DATA_ARRAY(Float32Array, data);
  INSTALL_DATA_ARRAY(Float64Array, data);

  Class_Int64Matrix = native_class(data, intern("Int64Matrix"),
                                   _Int64Matrix_init, _Int64Matrix_delete);
  native_method(Class_Int64Matrix, CONSTRUCTOR_KEY, _Int64Matrix_constructor);
  native_method(Class_Int64Matrix, ARRAYLIKE_INDEX_KEY, _Int64Matrix_index);
  native_method(Class_Int64Matrix, ARRAYLIKE_SET_KEY, _Int64Matrix_set);
  native_method(Class_Int64Matrix, intern("shape"), _Int64Matrix_shape);
}