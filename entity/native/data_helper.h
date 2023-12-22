#ifndef ENTITY_NATIVE_DATA_HELPER_H_
#define ENTITY_NATIVE_DATA_HELPER_H_

#include <math.h>

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

#define ARRAY_INITIAL_SIZE 16

#define DEFINE_DATA_ARRAY(class_name, type_name, to_entity_fn)                 \
  static Class *Class_##class_name = NULL;                                     \
                                                                               \
  DEFINE_ARRAYLIKE(class_name, type_name);                                     \
  IMPL_ARRAYLIKE(class_name, type_name);                                       \
                                                                               \
  void _##class_name##_init(Object *obj) { obj->_internal_obj = NULL; }        \
  void _##class_name##_delete(Object *obj) {                                   \
    class_name##_delete(obj->_internal_obj);                                   \
  }                                                                            \
                                                                               \
  type_name _to_##type_name(const Entity *e) {                                 \
    switch (e->type) {                                                         \
    case NONE:                                                                 \
      return 0L;                                                               \
    case OBJECT:                                                               \
      return (type_name)(uint64_t)e->obj;                                      \
    case PRIMITIVE:                                                            \
      switch (ptype(&e->pri)) {                                                \
      case PRIMITIVE_CHAR:                                                     \
        return (type_name)pchar(&e->pri);                                      \
      case PRIMITIVE_INT:                                                      \
        return pint(&e->pri);                                                  \
      case PRIMITIVE_FLOAT:                                                    \
        return (type_name)pfloat(&e->pri);                                     \
      default:                                                                 \
        FATALF("Unknown primitive type");                                      \
      }                                                                        \
    }                                                                          \
    FATALF("Unknown type");                                                    \
    return 0;                                                                  \
  }                                                                            \
                                                                               \
  Entity _##class_name##_constructor(Task *task, Context *ctx, Object *obj,    \
                                     Entity *args) {                           \
    int initial_size;                                                          \
    class_name *arr;                                                           \
    if (NULL == args || IS_NONE(args)) {                                       \
      initial_size = ARRAY_INITIAL_SIZE;                                       \
    } else if (IS_INT(args)) {                                                 \
      initial_size = pint(&args->pri);                                         \
    } else if (IS_CLASS(args, Class_Array)) {                                  \
      Array *arr = (Array *)args->obj->_internal_obj;                          \
      initial_size = Array_size(arr);                                          \
    } else {                                                                   \
      return raise_error(                                                      \
          task, ctx,                                                           \
          "Error argument must either not be present or be an Int");           \
    }                                                                          \
    obj->_internal_obj = arr = class_name##_create_sz(initial_size);           \
                                                                               \
    if (IS_INT(args)) {                                                        \
      class_name##_set(arr, initial_size - 1, 0);                              \
    } else if (IS_CLASS(args, Class_Array)) {                                  \
      Array *arr2 = (Array *)args->obj->_internal_obj;                         \
      for (int i = 0; i < Array_size(arr2); ++i) {                             \
        Entity *e = Array_get_ref(arr2, i);                                    \
        type_name ie = _to_##type_name(e);                                     \
        class_name##_set(arr, i, ie);                                          \
      }                                                                        \
    } else {                                                                   \
      memset(arr->table, 0x0, sizeof(type_name) * initial_size);               \
    }                                                                          \
    return entity_object(obj);                                                 \
  }                                                                            \
                                                                               \
  Entity _##class_name##_len(Task *task, Context *ctx, Object *obj,            \
                             Entity *args) {                                   \
    return entity_int(class_name##_size((class_name *)obj->_internal_obj));    \
  }                                                                            \
                                                                               \
  Entity _##class_name##_index_range(Task *task, Context *ctx,                 \
                                     class_name *self, _Range *range) {        \
    if (range->inc == 1) {                                                     \
      Object *obj = heap_new(task->parent_process->heap, Class_##class_name);  \
      size_t new_len = range->end - range->start;                              \
      obj->_internal_obj =                                                     \
          class_name##_create_copy(self->table + range->start, new_len);       \
      return entity_object(obj);                                               \
    } else {                                                                   \
      if (class_name##_size(self) < range->end || range->start < 0) {          \
        return raise_error(                                                    \
            task, ctx, "Range out of bounds: (%d:%d:%d) vs size=%d",           \
            range->start, range->end, range->inc, class_name##_size(self));    \
      }                                                                        \
      Object *obj = heap_new(task->parent_process->heap, Class_##class_name);  \
      size_t new_len = (range->end - range->start + 1) / range->inc;           \
      class_name *arr = class_name##_create_sz(new_len);                       \
      for (int i = range->start, j = 0;                                        \
           range->inc > 0 ? i < range->end : i > range->end;                   \
           i += range->inc, ++j) {                                             \
        class_name##_set(arr, j, class_name##_get(self, i));                   \
      }                                                                        \
      obj->_internal_obj = arr;                                                \
      return entity_object(obj);                                               \
    }                                                                          \
  }                                                                            \
                                                                               \
  Entity _##class_name##_index(Task *task, Context *ctx, Object *obj,          \
                               Entity *args) {                                 \
    ASSERT(NOT_NULL(args));                                                    \
    class_name *self = (class_name *)obj->_internal_obj;                       \
                                                                               \
    if (IS_CLASS(args, Class_Range)) {                                         \
      _Range *range = (_Range *)args->obj->_internal_obj;                      \
      return _##class_name##_index_range(task, ctx, self, range);              \
    }                                                                          \
                                                                               \
    if (PRIMITIVE != args->type || PRIMITIVE_INT != ptype(&args->pri)) {       \
      return raise_error(task, ctx, "Bad " #class_name " index");              \
    }                                                                          \
                                                                               \
    int32_t index = pint(&args->pri);                                          \
                                                                               \
    if (index < 0 || index >= class_name##_size(self)) {                       \
      return raise_error(task, ctx, "Index out of bounds: index=%d, size=%d",  \
                         index, class_name##_size(self));                      \
    }                                                                          \
                                                                               \
    return to_entity_fn(class_name##_get(self, index));                        \
  }                                                                            \
                                                                               \
  Entity _##class_name##_set_range(Task *task, Context *ctx, class_name *arr,  \
                                   _Range *range, const Entity *val) {         \
    class_name *value = (class_name *)val->obj->_internal_obj;                 \
    size_t expected_len = (range->end - range->start + 1) / range->inc;        \
    if (range->end > class_name##_size(arr) || range->start < 0) {             \
      return raise_error(task, ctx, "Invalid range: (%d:%d:%d) vs size=%d",    \
                         range->start, range->end, range->inc,                 \
                         class_name##_size(arr));                              \
    }                                                                          \
    if (expected_len != class_name##_size(value)) {                            \
      return raise_error(                                                      \
          task, ctx,                                                           \
          "Mismatch between left-hand and right-hand sizes: LH=%d, RH=%d",     \
          expected_len, class_name##_size(value));                             \
    }                                                                          \
    for (int i = range->start, j = 0;                                          \
         range->inc > 0 ? i < range->end : i > range->end;                     \
         i += range->inc, ++j) {                                               \
      class_name##_set(arr, i, class_name##_get(value, j));                    \
    }                                                                          \
    return *val;                                                               \
  }                                                                            \
                                                                               \
  Entity _##class_name##_set(Task *task, Context *ctx, Object *obj,            \
                             Entity *args) {                                   \
    class_name *arr = (class_name *)obj->_internal_obj;                        \
    if (!IS_TUPLE(args)) {                                                     \
      return raise_error(task, ctx, "Expected tuple input");                   \
    }                                                                          \
    Tuple *tupl_args = (Tuple *)args->obj->_internal_obj;                      \
    if (2 != tuple_size(tupl_args)) {                                          \
      return raise_error(task, ctx,                                            \
                         "Invalid number of arguments, expected 2, got %d",    \
                         tuple_size(tupl_args));                               \
    }                                                                          \
    const Entity *index = tuple_get(tupl_args, 0);                             \
    const Entity *val = tuple_get(tupl_args, 1);                               \
                                                                               \
    if (IS_CLASS(index, Class_Range)) {                                        \
      _Range *range = (_Range *)index->obj->_internal_obj;                     \
      if (!IS_CLASS(val, Class_##class_name)) {                                \
        return raise_error(task, ctx, "Value must be an " #class_name);        \
      }                                                                        \
      return _##class_name##_set_range(task, ctx, arr, range, val);            \
    } else if (IS_INT(index)) {                                                \
      int indexi = pint(&index->pri);                                          \
      if (indexi < 0 || indexi >= class_name##_size(arr)) {                    \
        return raise_error(task, ctx, "Index out of bounds");                  \
      }                                                                        \
      type_name ival = _to_##type_name(val);                                   \
      class_name##_set(arr, pint(&index->pri), ival);                          \
    } else {                                                                   \
      return raise_error(task, ctx, "Index must be int or Range.");            \
    }                                                                          \
    return *val;                                                               \
  }                                                                            \
                                                                               \
  Entity _##class_name##_to_arr(Task *task, Context *ctx, Object *obj,         \
                                Entity *args) {                                \
    ASSERT(NOT_NULL(args));                                                    \
    class_name *self = (class_name *)obj->_internal_obj;                       \
                                                                               \
    Object *arre = array_create(task->parent_process->heap);                   \
    Array *arr = (Array *)arre->_internal_obj;                                 \
                                                                               \
    for (int i = 0; i < class_name##_size(self); ++i) {                        \
      *Array_set_ref(arr, i) = to_entity_fn(class_name##_get(self, i));        \
    }                                                                          \
    return entity_object(arre);                                                \
  }                                                                            \
                                                                               \
  Entity _##class_name##_reversed(Task *task, Context *ctx, Object *obj,       \
                                  Entity *args) {                              \
    ASSERT(NOT_NULL(args));                                                    \
    class_name *self = (class_name *)obj->_internal_obj;                       \
                                                                               \
    Object *new_obj =                                                          \
        heap_new(task->parent_process->heap, Class_##class_name);              \
    int size = class_name##_size(self);                                        \
    class_name *arr = class_name##_create_sz(size);                            \
    for (int i = 0; i < size; ++i) {                                           \
      class_name##_set(arr, size - i - 1, class_name##_get(self, i));          \
    }                                                                          \
    new_obj->_internal_obj = arr;                                              \
    return entity_object(new_obj);                                             \
  }                                                                            \
                                                                               \
  void _##type_name##_swap(type_name *arr, int i, int j) {                     \
    type_name tmp = arr[i];                                                    \
    arr[i] = arr[j];                                                           \
    arr[j] = tmp;                                                              \
  }                                                                            \
                                                                               \
  int _##type_name##_partition(type_name *arr, int low, int high) {            \
    const int pivot = arr[low];                                                \
    int i = low - 1, j = high + 1;                                             \
    while (true) {                                                             \
      do {                                                                     \
        ++i;                                                                   \
      } while (arr[i] < pivot);                                                \
      do {                                                                     \
        --j;                                                                   \
      } while (arr[j] > pivot);                                                \
      if (i >= j) {                                                            \
        return j;                                                              \
      }                                                                        \
      _##type_name##_swap(arr, i, j);                                          \
    }                                                                          \
  }                                                                            \
                                                                               \
  int _##type_name##_partition_r(type_name *arr, int low, int high) {          \
    const int random = low + rand() % (high - low);                            \
    _##type_name##_swap(arr, random, low);                                     \
    return _##type_name##_partition(arr, low, high);                           \
  }                                                                            \
                                                                               \
  void _##type_name##_sort(type_name *arr, int low, int high) {                \
    if (low >= high) {                                                         \
      return;                                                                  \
    }                                                                          \
    const int pi = _##type_name##_partition_r(arr, low, high);                 \
    _##type_name##_sort(arr, low, pi);                                         \
    _##type_name##_sort(arr, pi + 1, high);                                    \
  }                                                                            \
                                                                               \
  Entity _##class_name##_sorted(Task *task, Context *ctx, Object *obj,         \
                                Entity *args) {                                \
    ASSERT(NOT_NULL(args));                                                    \
    class_name *self = (class_name *)obj->_internal_obj;                       \
                                                                               \
    Object *new_obj =                                                          \
        heap_new(task->parent_process->heap, Class_##class_name);              \
    int size = class_name##_size(self);                                        \
    class_name *arr = class_name##_copy(self);                                 \
    _##type_name##_sort(arr->table, 0, size - 1);                              \
    new_obj->_internal_obj = arr;                                              \
    return entity_object(new_obj);                                             \
  }                                                                            \
                                                                               \
  void _##class_name##_copy_fn(Heap *heap, Map *cpy_map, Object *target_obj,   \
                               Object *src_obj) {                              \
    class_name *src = (class_name *)src_obj->_internal_obj;                    \
    target_obj->_internal_obj = class_name##_copy(src);                        \
  }

#define DEFINE_DATA_MATRIX(class_name, array_class_name, type_name,            \
                           to_entity_fn)                                       \
  static Class *Class_##class_name = NULL;                                     \
                                                                               \
  typedef struct {                                                             \
    array_class_name *arr;                                                     \
    size_t dim1, dim2;                                                         \
  } class_name;                                                                \
                                                                               \
  void _##class_name##_init(Object *obj) { obj->_internal_obj = NULL; }        \
  void _##class_name##_delete(Object *obj) {                                   \
    array_class_name##_delete(((class_name *)obj->_internal_obj)->arr);        \
    DEALLOC(obj->_internal_obj);                                               \
  }                                                                            \
                                                                               \
  class_name *_##class_name##_create(int dim1, int dim2, bool clear) {         \
    class_name *mat = ALLOC2(class_name);                                      \
    mat->dim1 = dim1;                                                          \
    mat->dim2 = dim2;                                                          \
    mat->arr = array_class_name##_create_sz(dim1 * dim2);                      \
    if (clear) {                                                               \
      memset(mat->arr->table, 0x0, sizeof(type_name) * dim1 * dim2);           \
    }                                                                          \
    return mat;                                                                \
  }                                                                            \
                                                                               \
  Entity _##class_name##_constructor(Task *task, Context *ctx, Object *obj,    \
                                     Entity *args) {                           \
    if (IS_TUPLE(args)) {                                                      \
      Tuple *tupl_args = (Tuple *)args->obj->_internal_obj;                    \
      if (2 != tuple_size(tupl_args)) {                                        \
        return raise_error(task, ctx,                                          \
                           "Invalid number of arguments, expected 2, got %d",  \
                           tuple_size(tupl_args));                             \
      }                                                                        \
      const Entity *e_dim1 = tuple_get(tupl_args, 0);                          \
      const Entity *e_dim2 = tuple_get(tupl_args, 1);                          \
                                                                               \
      if (!IS_INT(e_dim1) || !IS_INT(e_dim2)) {                                \
        return raise_error(task, ctx, "Invalid input: expected (Int, Int).");  \
      }                                                                        \
                                                                               \
      const size_t dim1 = pint(&e_dim1->pri);                                  \
      const size_t dim2 = pint(&e_dim2->pri);                                  \
                                                                               \
      obj->_internal_obj = _##class_name##_create(dim1, dim2, /*clear=*/true); \
    } else if (IS_ARRAY(args)) {                                               \
      Array *arr = (Array *)args->obj->_internal_obj;                          \
      const size_t dim1 = Array_size(arr);                                     \
      int max_dim2 = 0;                                                        \
      for (int i = 0; i < Array_size(arr); ++i) {                              \
        Entity *e = Array_get_ref(arr, i);                                     \
        if (IS_ARRAY(e)) {                                                     \
          max_dim2 =                                                           \
              max(max_dim2, Array_size((Array *)e->obj->_internal_obj));       \
        } else if (IS_CLASS(e, Class_##array_class_name)) {                    \
          max_dim2 =                                                           \
              max(max_dim2, array_class_name##_size(                           \
                                (array_class_name *)e->obj->_internal_obj));   \
        } else {                                                               \
          return raise_error(task, ctx,                                        \
                             "Invalid member of input array at index %d", i);  \
        }                                                                      \
      }                                                                        \
      class_name *mat;                                                         \
      obj->_internal_obj = mat =                                               \
          _##class_name##_create(dim1, max_dim2, /*clear=*/true);              \
                                                                               \
      for (int i = 0; i < Array_size(arr); ++i) {                              \
        Entity *e = Array_get_ref(arr, i);                                     \
        if (IS_ARRAY(e)) {                                                     \
          Array *arri = (Array *)e->obj->_internal_obj;                        \
          for (int j = 0; j < Array_size(arri); ++j) {                         \
            type_name val = pint(&Array_get_ref(arri, j)->pri);                \
            mat->arr->table[i * mat->dim2 + j] = val;                          \
          }                                                                    \
        } else /*if (IS_CLASS(e, Class_##array_class_name))*/ {                \
          const array_class_name *arri =                                       \
              (array_class_name *)e->obj->_internal_obj;                       \
          memmove(mat->arr->table + i * mat->dim2, arri->table,                \
                  sizeof(type_name) * mat->dim2);                              \
        }                                                                      \
      }                                                                        \
    }                                                                          \
    return entity_object(obj);                                                 \
  }                                                                            \
                                                                               \
  Entity _##class_name##_shape(Task *task, Context *ctx, Object *obj,          \
                               Entity *args) {                                 \
    const class_name *mat = (class_name *)obj->_internal_obj;                  \
    Entity dim1 = entity_int(mat->dim1);                                       \
    Entity dim2 = entity_int(mat->dim2);                                       \
    Object *shape = tuple_create2(task->parent_process->heap, &dim1, &dim2);   \
    return entity_object(shape);                                               \
  }                                                                            \
                                                                               \
  size_t _##class_name##_get_dim(const class_name *mat, int dim) {             \
    return dim == 1 ? mat->dim1 : mat->dim2;                                   \
  }                                                                            \
                                                                               \
  int _##class_name##_first_bad_index(const class_name *mat, int dim,          \
                                      const Tuple *dim_indices) {              \
    for (int i = 0; i < tuple_size(dim_indices); ++i) {                        \
      const Entity *e = tuple_get(dim_indices, i);                             \
      if (!IS_INT(e) || pint(&e->pri) < 0 ||                                   \
          pint(&e->pri) >= _##class_name##_get_dim(mat, dim)) {                \
        return i;                                                              \
      }                                                                        \
    }                                                                          \
    return -1;                                                                 \
  }                                                                            \
                                                                               \
  bool _##class_name##_range_is_valid(const class_name *mat, int dim,          \
                                      const _Range *range) {                   \
    return range->start >= 0 &&                                                \
           range->end <= _##class_name##_get_dim(mat, dim);                    \
  }                                                                            \
                                                                               \
  bool _##class_name##_index_is_valid(const class_name *mat, int dim,          \
                                      int index) {                             \
    return index >= 0 && index < _##class_name##_get_dim(mat, dim);            \
  }                                                                            \
                                                                               \
  Entity _##class_name##_compute_indices(                                      \
      Task *task, Context *ctx, const Entity *arg, const class_name *mat,      \
      int dim, int *dim_index, Tuple **dim_indices, _Range **dim_range) {      \
    if (IS_INT(arg)) {                                                         \
      *dim_index = pint(&arg->pri);                                            \
      if (!_##class_name##_index_is_valid(mat, dim, *dim_index)) {             \
        return raise_error(task, ctx,                                          \
                           "Dim%d out of bounds: %d, must be in [0, %d)", dim, \
                           *dim_index, _##class_name##_get_dim(mat, dim));     \
      }                                                                        \
    } else if (IS_TUPLE(arg)) {                                                \
      *dim_indices = (Tuple *)arg->obj->_internal_obj;                         \
      int bad_index = -1;                                                      \
      if ((bad_index = _##class_name##_first_bad_index(mat, dim,               \
                                                       *dim_indices)) >= 0) {  \
        return raise_error(task, ctx, "Invalid dim%d index at %d", dim,        \
                           bad_index);                                         \
      }                                                                        \
    } else if (IS_CLASS(arg, Class_Range)) {                                   \
      *dim_range = (_Range *)arg->obj->_internal_obj;                          \
      if (!_##class_name##_range_is_valid(mat, dim, *dim_range)) {             \
        return raise_error(                                                    \
            task, ctx, "Dim%d out of bounds: (%d:%d:%d), must be in [0, %d)",  \
            dim, (*dim_range)->start, (*dim_range)->end, (*dim_range)->inc,    \
            _##class_name##_get_dim(mat, dim));                                \
      }                                                                        \
    } else {                                                                   \
      return raise_error(                                                      \
          task, ctx, "Expected dim%d arg to be an Int, Range, or tuple", dim); \
    }                                                                          \
    return NONE_ENTITY;                                                        \
  }                                                                            \
                                                                               \
  type_name _##class_name##_get_value(const class_name *mat, int dim1_index,   \
                                      int dim2_index) {                        \
    return array_class_name##_get(mat->arr,                                    \
                                  mat->dim2 * dim1_index + dim2_index);        \
  }                                                                            \
                                                                               \
  void _##class_name##_set_value(const class_name *mat, int dim1_index,        \
                                 int dim2_index, type_name val) {              \
    array_class_name##_set(mat->arr, mat->dim2 *dim1_index + dim2_index, val); \
  }                                                                            \
                                                                               \
  size_t _##class_name##_range_size(const _Range *range) {                     \
    return ceil(1.0 * (range->end - range->start) / range->inc);               \
  }                                                                            \
                                                                               \
  Entity _##class_name##_index(Task *task, Context *ctx, Object *obj,          \
                               Entity *args) {                                 \
    const class_name *mat = (class_name *)obj->_internal_obj;                  \
    if (IS_TUPLE(args)) {                                                      \
      Tuple *targs = (Tuple *)args->obj->_internal_obj;                        \
      if (tuple_size(targs) != 2) {                                            \
        return raise_error(task, ctx, "Expected exactly 2 args");              \
      }                                                                        \
      const Entity *first = tuple_get(targs, 0);                               \
      const Entity *second = tuple_get(targs, 1);                              \
                                                                               \
      int dim1_index = -1, dim2_index = -1;                                    \
      Tuple *dim1_indices = NULL, *dim2_indices = NULL;                        \
      _Range *dim1_range = NULL, *dim2_range = NULL;                           \
                                                                               \
      const Entity dim1_e = _##class_name##_compute_indices(                   \
          task, ctx, first, mat, 1, &dim1_index, &dim1_indices, &dim1_range);  \
      if (!IS_NONE(&dim1_e)) {                                                 \
        return dim1_e;                                                         \
      }                                                                        \
                                                                               \
      const Entity dim2_e = _##class_name##_compute_indices(                   \
          task, ctx, second, mat, 2, &dim2_index, &dim2_indices, &dim2_range); \
      if (!IS_NONE(&dim2_e)) {                                                 \
        return dim2_e;                                                         \
      }                                                                        \
                                                                               \
      if (dim1_index >= 0) {                                                   \
        if (dim2_index >= 0) {                                                 \
          return to_entity_fn(                                                 \
              _##class_name##_get_value(mat, dim1_index, dim2_index));         \
        } else if (NULL != dim2_indices) {                                     \
          array_class_name *new_arr;                                           \
          Object *new_obj =                                                    \
              heap_new(task->parent_process->heap, Class_##array_class_name);  \
          new_obj->_internal_obj = new_arr =                                   \
              array_class_name##_create_sz(tuple_size(dim2_indices));          \
          for (int i = 0; i < tuple_size(dim2_indices); ++i) {                 \
            array_class_name##_set(                                            \
                new_arr, i,                                                    \
                _##class_name##_get_value(                                     \
                    mat, dim1_index, pint(&tuple_get(dim2_indices, i)->pri))); \
          }                                                                    \
          return entity_object(new_obj);                                       \
        } else /* (NULL != dim2_range) */ {                                    \
          array_class_name *new_arr;                                           \
          Object *new_obj =                                                    \
              heap_new(task->parent_process->heap, Class_##array_class_name);  \
          new_obj->_internal_obj = new_arr = array_class_name##_create_sz(     \
              _##class_name##_range_size(dim2_range));                         \
          for (int i = dim2_range->start, j = 0;                               \
               dim2_range->inc > 0 ? i < dim2_range->end                       \
                                   : i > dim2_range->end;                      \
               i += dim2_range->inc, ++j) {                                    \
            array_class_name##_set(                                            \
                new_arr, j, _##class_name##_get_value(mat, dim1_index, i));    \
          }                                                                    \
          return entity_object(new_obj);                                       \
        }                                                                      \
      } else if (NULL != dim1_indices) {                                       \
        if (dim2_index >= 0) {                                                 \
          array_class_name *new_arr;                                           \
          Object *new_obj =                                                    \
              heap_new(task->parent_process->heap, Class_##array_class_name);  \
          new_obj->_internal_obj = new_arr =                                   \
              array_class_name##_create_sz(tuple_size(dim1_indices));          \
          for (int i = 0; i < tuple_size(dim1_indices); ++i) {                 \
            array_class_name##_set(                                            \
                new_arr, i,                                                    \
                _##class_name##_get_value(                                     \
                    mat, pint(&tuple_get(dim1_indices, i)->pri), dim2_index)); \
          }                                                                    \
          return entity_object(new_obj);                                       \
        } else if (NULL != dim2_indices) {                                     \
          class_name *new_mat;                                                 \
          Object *new_obj =                                                    \
              heap_new(task->parent_process->heap, Class_##class_name);        \
          new_obj->_internal_obj = new_mat = _##class_name##_create(           \
              tuple_size(dim1_indices), tuple_size(dim2_indices),              \
              /*clear=*/false);                                                \
          for (int i = 0; i < tuple_size(dim1_indices); ++i) {                 \
            for (int j = 0; j < tuple_size(dim2_indices); ++j) {               \
              array_class_name##_set(                                          \
                  new_mat->arr, i * new_mat->dim2 + j,                         \
                  _##class_name##_get_value(                                   \
                      mat, pint(&tuple_get(dim1_indices, i)->pri),             \
                      pint(&tuple_get(dim2_indices, j)->pri)));                \
            }                                                                  \
          }                                                                    \
          return entity_object(new_obj);                                       \
        } else /* (NULL != dim2_range) */ {                                    \
          class_name *new_mat;                                                 \
          Object *new_obj =                                                    \
              heap_new(task->parent_process->heap, Class_##class_name);        \
          new_obj->_internal_obj = new_mat = _##class_name##_create(           \
              tuple_size(dim1_indices),                                        \
              _##class_name##_range_size(dim2_range), /*clear=*/false);        \
          for (int i = 0; i < tuple_size(dim1_indices); ++i) {                 \
            for (int i2 = dim2_range->start, j = 0;                            \
                 dim2_range->inc > 0 ? i2 < dim2_range->end                    \
                                     : i2 > dim2_range->end;                   \
                 i2 += dim2_range->inc, ++j) {                                 \
              array_class_name##_set(                                          \
                  new_mat->arr, i * new_mat->dim2 + j,                         \
                  _##class_name##_get_value(                                   \
                      mat, pint(&tuple_get(dim1_indices, i)->pri), i2));       \
            }                                                                  \
          }                                                                    \
          return entity_object(new_obj);                                       \
        }                                                                      \
      } else /* if (NULL != dim1_range) */ {                                   \
        if (dim2_index >= 0) {                                                 \
          array_class_name *new_arr;                                           \
          Object *new_obj =                                                    \
              heap_new(task->parent_process->heap, Class_##array_class_name);  \
          new_obj->_internal_obj = new_arr = array_class_name##_create_sz(     \
              _##class_name##_range_size(dim1_range));                         \
          for (int i = dim1_range->start, j = 0;                               \
               dim1_range->inc > 0 ? i < dim1_range->end                       \
                                   : i > dim1_range->end;                      \
               i += dim1_range->inc, ++j) {                                    \
            array_class_name##_set(                                            \
                new_arr, j, _##class_name##_get_value(mat, i, dim2_index));    \
          }                                                                    \
          return entity_object(new_obj);                                       \
        } else if (NULL != dim2_indices) {                                     \
          class_name *new_mat;                                                 \
          Object *new_obj =                                                    \
              heap_new(task->parent_process->heap, Class_##class_name);        \
          new_obj->_internal_obj = new_mat = _##class_name##_create(           \
              _##class_name##_range_size(dim1_range),                          \
              tuple_size(dim2_indices), /*clear=*/false);                      \
          for (int i = 0; i < tuple_size(dim2_indices); ++i) {                 \
            for (int i2 = dim1_range->start, j = 0;                            \
                 dim1_range->inc > 0 ? i2 < dim1_range->end                    \
                                     : i2 > dim1_range->end;                   \
                 i2 += dim1_range->inc, ++j) {                                 \
              array_class_name##_set(                                          \
                  new_mat->arr, j * new_mat->dim2 + i,                         \
                  _##class_name##_get_value(                                   \
                      mat, i2, pint(&tuple_get(dim2_indices, i)->pri)));       \
            }                                                                  \
          }                                                                    \
          return entity_object(new_obj);                                       \
        } else /* (NULL != dim2_range) */ {                                    \
          class_name *new_mat;                                                 \
          Object *new_obj =                                                    \
              heap_new(task->parent_process->heap, Class_##class_name);        \
          new_obj->_internal_obj = new_mat = _##class_name##_create(           \
              _##class_name##_range_size(dim1_range),                          \
              _##class_name##_range_size(dim2_range), /*clear=*/false);        \
          for (int i = dim1_range->start, j = 0;                               \
               dim1_range->inc > 0 ? i < dim1_range->end                       \
                                   : i > dim1_range->end;                      \
               i += dim1_range->inc, ++j) {                                    \
            for (int k = dim2_range->start, l = 0;                             \
                 dim2_range->inc > 0 ? k < dim2_range->end                     \
                                     : k > dim2_range->end;                    \
                 k += dim2_range->inc, ++l) {                                  \
              array_class_name##_set(new_mat->arr, j * new_mat->dim2 + l,      \
                                     _##class_name##_get_value(mat, i, k));    \
            }                                                                  \
          }                                                                    \
          return entity_object(new_obj);                                       \
        }                                                                      \
      }                                                                        \
    } else if (IS_INT(args)) {                                                 \
      const int dim1_index = (int)pint(&args->pri);                            \
      if (!_##class_name##_index_is_valid(mat, 1, dim1_index)) {               \
        return raise_error(task, ctx,                                          \
                           "Index out of bounds: was %d, must be in [0,%d)",   \
                           dim1_index, mat->dim1);                             \
      }                                                                        \
      Object *subarr =                                                         \
          heap_new(task->parent_process->heap, Class_##array_class_name);      \
      subarr->_internal_obj = array_class_name##_create_copy(                  \
          mat->arr->table + dim1_index * mat->dim2, mat->dim2);                \
      return entity_object(subarr);                                            \
    } else if (IS_CLASS(args, Class_Range)) {                                  \
      const _Range *range = (_Range *)args->obj->_internal_obj;                \
      if (!_##class_name##_range_is_valid(mat, 1, range)) {                    \
        return raise_error(task, ctx,                                          \
                           "Range out of bounds: (%d:%d:%d) vs size=%d",       \
                           range->start, range->end, range->inc, mat->dim1);   \
      }                                                                        \
      const size_t new_dim1 = _##class_name##_range_size(range);               \
                                                                               \
      Object *submat_o =                                                       \
          heap_new(task->parent_process->heap, Class_##class_name);            \
      class_name *submat;                                                      \
      submat_o->_internal_obj = submat =                                       \
          _##class_name##_create(new_dim1, mat->dim2, /*clear=*/false);        \
      for (int i = range->start, j = 0;                                        \
           range->inc > 0 ? i < range->end : i > range->end;                   \
           i += range->inc, ++j) {                                             \
        memmove(submat->arr->table + j * mat->dim2,                            \
                mat->arr->table + i * mat->dim2,                               \
                sizeof(type_name) * mat->dim2);                                \
      }                                                                        \
      return entity_object(submat_o);                                          \
    } else {                                                                   \
      return raise_error(task, ctx, "Expected tuple arg.");                    \
    }                                                                          \
    return NONE_ENTITY;                                                        \
  }                                                                            \
                                                                               \
  void _##class_name##_add_indices(Task *task, Context *ctx,                   \
                                   const Entity *arg, const class_name *mat,   \
                                   int dim, AList *indices, Entity *error) {   \
    int index = -1;                                                            \
    Tuple *tuple = NULL;                                                       \
    _Range *range = NULL;                                                      \
    Entity e = _##class_name##_compute_indices(task, ctx, arg, mat, dim,       \
                                               &index, &tuple, &range);        \
    if (!IS_NONE(&e)) {                                                        \
      *error = e;                                                              \
      return;                                                                  \
    }                                                                          \
                                                                               \
    if (index >= 0) {                                                          \
      *(int *)alist_add(indices) = index;                                      \
    } else if (NULL != tuple) {                                                \
      for (int i = 0; i < tuple_size(tuple); ++i) {                            \
        *(int *)alist_add(indices) = (int)pint(&tuple_get(tuple, i)->pri);     \
      }                                                                        \
    } else if (NULL != range) {                                                \
      for (int i = range->start, j = 0;                                        \
           range->inc > 0 ? i < range->end : i > range->end;                   \
           i += range->inc, ++j) {                                             \
        *(int *)alist_add(indices) = i;                                        \
      }                                                                        \
    } else {                                                                   \
      *error = raise_error(task, ctx, "Invalid dimension type");               \
    }                                                                          \
  }                                                                            \
                                                                               \
  Entity _##class_name##_set(Task *task, Context *ctx, Object *obj,            \
                             Entity *args) {                                   \
    const class_name *mat = (class_name *)obj->_internal_obj;                  \
                                                                               \
    if (!IS_TUPLE(args)) {                                                     \
      return raise_error(task, ctx, "Expected tuple input");                   \
    }                                                                          \
    Tuple *targs = (Tuple *)args->obj->_internal_obj;                          \
    if (tuple_size(targs) != 2) {                                              \
      return raise_error(task, ctx, "Expected exactly 2 args");                \
    }                                                                          \
    const Entity *lhs = tuple_get(targs, 0);                                   \
    const Entity *rhs = tuple_get(targs, 1);                                   \
                                                                               \
    if (!IS_TUPLE(lhs)) {                                                      \
      return raise_error(task, ctx, "Expected tuple lhs");                     \
    }                                                                          \
    Tuple *dims = (Tuple *)lhs->obj->_internal_obj;                            \
    if (tuple_size(dims) != 2) {                                               \
      return raise_error(task, ctx, "Expected lhs to be Tuple(2)");            \
    }                                                                          \
                                                                               \
    if (!IS_CLASS(rhs, Class_##class_name)) {                                  \
      return raise_error(task, ctx, "Expected rhs to be class_name");          \
    }                                                                          \
    const class_name *rhs_mat = (class_name *)rhs->obj->_internal_obj;         \
                                                                               \
    AList dim1_indices, dim2_indices;                                          \
    alist_init(&dim1_indices, int, 8);                                         \
    alist_init(&dim2_indices, int, 8);                                         \
                                                                               \
    Entity error = NONE_ENTITY;                                                \
                                                                               \
    _##class_name##_add_indices(task, ctx, tuple_get(dims, 0), mat, 1,         \
                                &dim1_indices, &error);                        \
    if (!IS_NONE(&error)) {                                                    \
      return error;                                                            \
    }                                                                          \
    if (rhs_mat->dim1 != alist_len(&dim1_indices)) {                           \
      return raise_error(task, ctx,                                            \
                         "Dimension mismatch. lhs dim1=%d, rhs dim1=%d",       \
                         alist_len(&dim1_indices), rhs_mat->dim1);             \
    }                                                                          \
                                                                               \
    _##class_name##_add_indices(task, ctx, tuple_get(dims, 1), mat, 2,         \
                                &dim2_indices, &error);                        \
    if (!IS_NONE(&error)) {                                                    \
      return error;                                                            \
    }                                                                          \
    if (rhs_mat->dim2 != alist_len(&dim2_indices)) {                           \
      return raise_error(task, ctx,                                            \
                         "Dimension mismatch. lhs dim2=%d, rhs dim2=%d",       \
                         alist_len(&dim2_indices), rhs_mat->dim2);             \
    }                                                                          \
                                                                               \
    for (int i = 0; i < alist_len(&dim1_indices); ++i) {                       \
      for (int j = 0; j < alist_len(&dim2_indices); ++j) {                     \
        const int dim1_index = *(int *)alist_get(&dim1_indices, i);            \
        const int dim2_index = *(int *)alist_get(&dim2_indices, j);            \
        _##class_name##_set_value(mat, dim1_index, dim2_index,                 \
                                  _##class_name##_get_value(rhs_mat, i, j));   \
      }                                                                        \
    }                                                                          \
                                                                               \
    alist_finalize(&dim1_indices);                                             \
    alist_finalize(&dim2_indices);                                             \
                                                                               \
    return NONE_ENTITY;                                                        \
  }

#define INSTALL_DATA_ARRAY(class_name, data)                                   \
  {                                                                            \
    Class_##class_name =                                                       \
        native_class(data, intern(#class_name), _##class_name##_init,          \
                     _##class_name##_delete);                                  \
    Class_##class_name->_copy_fn = (ObjCopyFn)_##class_name##_copy_fn;         \
    native_method(Class_##class_name, CONSTRUCTOR_KEY,                         \
                  _##class_name##_constructor);                                \
    native_method(Class_##class_name, intern("len"), _##class_name##_len);     \
    native_method(Class_##class_name, ARRAYLIKE_INDEX_KEY,                     \
                  _##class_name##_index);                                      \
    native_method(Class_##class_name, ARRAYLIKE_SET_KEY, _##class_name##_set); \
    native_method(Class_##class_name, intern("to_arr"),                        \
                  _##class_name##_to_arr);                                     \
    native_method(Class_##class_name, intern("reversed"),                      \
                  _##class_name##_reversed);                                   \
    native_method(Class_##class_name, intern("sorted"),                        \
                  _##class_name##_sorted);                                     \
  }

#define INSTALL_DATA_MATRIX(class_name, data)                                  \
  {                                                                            \
    Class_##class_name =                                                       \
        native_class(data, intern(#class_name), _##class_name##_init,          \
                     _##class_name##_delete);                                  \
    native_method(Class_##class_name, CONSTRUCTOR_KEY,                         \
                  _##class_name##_constructor);                                \
    native_method(Class_##class_name, ARRAYLIKE_INDEX_KEY,                     \
                  _##class_name##_index);                                      \
    native_method(Class_##class_name, ARRAYLIKE_SET_KEY, _##class_name##_set); \
    native_method(Class_##class_name, intern("shape"), _##class_name##_shape); \
  }

#endif /* ENTITY_NATIVE_DATA_HELPER_H_ */