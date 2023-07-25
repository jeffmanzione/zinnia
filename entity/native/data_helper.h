#ifndef ENTITY_NATIVE_DATA_HELPER_H_
#define ENTITY_NATIVE_DATA_HELPER_H_

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
    return entity_int(class_name##_size((class_name##*)obj->_internal_obj));   \
  }                                                                            \
                                                                               \
  Entity _##class_name##_index_range(Task *task, Context *ctx,                 \
                                     class_name *self, _Range *range) {        \
    Object *obj = heap_new(task->parent_process->heap, Class_##class_name##);  \
    if (range->inc == 1) {                                                     \
      size_t new_len = range->end - range->start;                              \
      obj->_internal_obj =                                                     \
          class_name##_create_copy(self->table + range->start, new_len);       \
    } else {                                                                   \
      size_t new_len = (range->end - range->start + 1) / range->inc;           \
      class_name *arr = class_name##_create_sz(new_len);                       \
      for (int i = range->start, j = 0;                                        \
           range->inc > 0 ? i < range->end : i > range->end;                   \
           i += range->inc, ++j) {                                             \
        if (class_name##_size(self) <= i) {                                    \
          return raise_error(task, ctx,                                        \
                             "Index out of bounds: index=%d, size=%d", i,      \
                             class_name##_size(self));                         \
        }                                                                      \
        class_name##_set(arr, j, class_name##_get(self, i));                   \
      }                                                                        \
      obj->_internal_obj = arr;                                                \
    }                                                                          \
    return entity_object(obj);                                                 \
  }                                                                            \
                                                                               \
  Entity _##class_name##_index(Task *task, Context *ctx, Object *obj,          \
                               Entity *args) {                                 \
    ASSERT(NOT_NULL(args));                                                    \
    class_name *self = (class_name##*)obj->_internal_obj;                      \
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
    class_name *arr = (class_name##*)obj->_internal_obj;                       \
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
    class_name *self = (class_name##*)obj->_internal_obj;                      \
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
  inline void _##type_name##_swap(type_name *arr, int i, int j) {              \
    type_name tmp = arr[i];                                                    \
    arr[i] = arr[j];                                                           \
    arr[j] = tmp;                                                              \
  }                                                                            \
                                                                               \
  inline int _##type_name##_partition(type_name *arr, int low, int high) {     \
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
  inline int _##type_name##_partition_r(type_name *arr, int low, int high) {   \
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

#define INSTALL_DATA_ARRAY(class_name, data)                                   \
  {                                                                            \
    Class_##class_name =                                                       \
        native_class(data, intern(#class_name), _##class_name##_init,          \
                     _##class_name##_delete);                                  \
    Class_##class_name->_copy_fn = _##class_name##_copy_fn;                    \
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

#endif /* ENTITY_NATIVE_DATA_HELPER_H_ */