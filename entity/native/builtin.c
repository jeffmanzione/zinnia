// builtin.c
//
// Created on: Aug 29, 2020
//     Author: Jeff Manzione

#include "entity/native/builtin.h"

#include <inttypes.h>

#include "alloc/alloc.h"
#include "alloc/arena/intern.h"
#include "entity/array/array.h"
#include "entity/class/classes.h"
#include "entity/class/classes_def.h"
#include "entity/entity.h"
#include "entity/native/async.h"
#include "entity/native/error.h"
#include "entity/native/native.h"
#include "entity/object.h"
#include "entity/primitive.h"
#include "entity/string/string.h"
#include "entity/string/string_helper.h"
#include "entity/tuple/tuple.h"
#include "heap/heap.h"
#include "struct/map.h"
#include "struct/struct_defaults.h"
#include "util/file/file_info.h"
#include "util/string.h"
#include "util/util.h"
#include "vm/intern.h"
#include "vm/process/context.h"
#include "vm/process/process.h"
#include "vm/process/processes.h"
#include "vm/process/remote.h"
#include "vm/process/task.h"
#include "vm/vm.h"

// From vm/virtual_machine.h
ThreadHandle process_run_in_new_thread(Process *process);

#ifndef min
#define min(x, y) ((x) > (y) ? (y) : (x))
#endif

#define BUFFER_SIZE 256
#define INFER_FROM_STRING 0

bool _is_any_space(const char c) {
  switch (c) {
  case ' ':
  case '\t':
  case '\n':
  case '\r':
    return true;
  default:
    return false;
  }
}

static Class *Class_Range;

typedef struct {
  int32_t start;
  int32_t end;
  int32_t inc;
} _Range;

bool _str_to_int64(String *str, int64_t *result) {
  char *cstr = ALLOC_STRNDUP(str->table, String_size(str));
  char *endptr;
  *result = strtol(cstr, &endptr, INFER_FROM_STRING);
  // Error scenario.
  if (0 == result && endptr - cstr != String_size(str)) {
    DEALLOC(cstr);
    return false;
  }
  DEALLOC(cstr);
  return true;
}

Entity _Int(Task *task, Context *ctx, Object *obj, Entity *args) {
  int64_t result;
  if (NULL == args) {
    return entity_int(0);
  }
  switch (args->type) {
  case NONE:
    return entity_int(0);
  case OBJECT:
    if (!IS_CLASS(args, Class_String)) {
      return raise_error(task, ctx, "Cannot convert input to Int.");
    }
    if (!_str_to_int64((String *)args->obj->_internal_obj, &result)) {
      return raise_error(task, ctx, "Cannot convert input '%*s' to Int.",
                         String_size((String *)args->obj->_internal_obj),
                         args->obj->_internal_obj);
    }
    return entity_int(result);
  case PRIMITIVE:
    switch (ptype(&args->pri)) {
    case PRIMITIVE_CHAR:
      return entity_int((int64_t)pchar(&args->pri));
    case PRIMITIVE_INT:
      return *args;
    case PRIMITIVE_FLOAT:
      return entity_int((int64_t)pfloat(&args->pri));
    default:
      return raise_error(task, ctx, "Unknown primitive type.");
    }
  default:
    return raise_error(task, ctx, "Unknown type.");
  }
  return entity_int(0);
}

bool _str_to_float(String *str, double *result) {
  char *cstr = ALLOC_STRNDUP(str->table, String_size(str));
  char *endptr;
  *result = strtod(cstr, &endptr);
  // Error scenario.
  if (0 == result && endptr - cstr != String_size(str)) {
    DEALLOC(cstr);
    return false;
  }
  DEALLOC(cstr);
  return true;
}

Entity _Float(Task *task, Context *ctx, Object *obj, Entity *args) {
  double result;
  if (NULL == args) {
    return entity_int(0);
  }
  switch (args->type) {
  case NONE:
    return entity_float(0.f);
  case OBJECT:
    if (!IS_CLASS(args, Class_String)) {
      return raise_error(task, ctx, "Cannot convert input to Float.");
    }
    if (!_str_to_float((String *)args->obj->_internal_obj, &result)) {
      return raise_error(task, ctx, "Cannot convert input '%*s' to Float.",
                         String_size((String *)args->obj->_internal_obj),
                         args->obj->_internal_obj);
    }
    return entity_float(result);
  case PRIMITIVE:
    switch (ptype(&args->pri)) {
    case PRIMITIVE_CHAR:
      return entity_float((double)pchar(&args->pri));
    case PRIMITIVE_INT:
      return entity_float((double)pint(&args->pri));
    case PRIMITIVE_FLOAT:
      return *args;
    default:
      return raise_error(task, ctx, "Unknown primitive type.");
    }
  default:
    return raise_error(task, ctx, "Unknown type.");
  }
  return entity_float(0.f);
}

bool _str_to_bool(String *str, bool *result) {
  char *cstr = ALLOC_STRNDUP(str->table, String_size(str));
  if (0 == strcmp("True", cstr) || 0 == strcmp("true", cstr) ||
      0 == strcmp("T", cstr) || 0 == strcmp("t", cstr)) {
    *result = true;
    DEALLOC(cstr);
    return true;
  }
  if (0 == strcmp("False", cstr) || 0 == strcmp("false", cstr) ||
      0 == strcmp("F", cstr) || 0 == strcmp("f", cstr)) {
    *result = false;
    DEALLOC(cstr);
    return true;
  }
  DEALLOC(cstr);
  return false;
}

Entity __Bool(Task *task, Context *ctx, Object *obj, Entity *args) {
  bool result;
  if (NULL == args) {
    return NONE_ENTITY;
  }
  switch (args->type) {
  case NONE:
    return NONE_ENTITY;
  case OBJECT:
    if (!IS_CLASS(args, Class_String)) {
      return raise_error(task, ctx, "Cannot convert input to bool Int.");
    }
    if (!_str_to_bool((String *)args->obj->_internal_obj, &result)) {
      return raise_error(task, ctx, "Cannot convert input '%*s' to bool Int.",
                         String_size((String *)args->obj->_internal_obj),
                         args->obj->_internal_obj);
    }
    return result ? entity_int(1) : NONE_ENTITY;
  case PRIMITIVE:
    switch (ptype(&args->pri)) {
    case PRIMITIVE_CHAR:
    case PRIMITIVE_INT:
    case PRIMITIVE_FLOAT:
      return entity_int(1);
    default:
      return raise_error(task, ctx, "Unknown primitive type.");
    }
  default:
    return raise_error(task, ctx, "Unknown type.");
  }
  return NONE_ENTITY;
}

Object *_wrap_function_in_ref2(const Function *f, Object *obj, Task *task,
                               Context *ctx) {
  Object *fn_ref = heap_new(task->parent_process->heap, Class_FunctionRef);
  __function_ref_init(fn_ref, obj, f, f->_is_anon ? ctx : NULL);
  return fn_ref;
}

volatile int tmp = 0;

Entity _collect_garbage(Task *task, Context *ctx, Object *obj, Entity *args) {
  Process *process = task->parent_process;
  Heap *heap = process->heap;
  uint32_t deleted_nodes_count = process_collect_garbage(process);

  // printf("Tasks:\n\titem_size=%u\n\tcapacity=%u\n\titem_count=%u\n\tsubarena_"
  //        "capacity=%u\n\tsubarena_count=%u\n",
  //        __arena_item_size(&process->task_arena),
  //        __arena_capacity(&process->task_arena),
  //        __arena_item_count(&process->task_arena),
  //        __arena_subarena_capacity(&process->task_arena),
  //        __arena_subarena_count(&process->task_arena));

  // printf(
  //     "Contexts:\n\titem_size=%u\n\tcapacity=%u\n\titem_count=%u\n\tsubarena_"
  //     "capacity=%u\n\tsubarena_count=%u\n",
  //     __arena_item_size(&process->context_arena),
  //     __arena_capacity(&process->context_arena),
  //     __arena_item_count(&process->context_arena),
  //     __arena_subarena_capacity(&process->context_arena),
  //     __arena_subarena_count(&process->context_arena));

  Object *object_counts = array_create(heap);

  HeapProfile *hp = heap_create_profile(heap);
  M_iter iter = heapprofile_object_type_counts(hp);
  for (; has(&iter); inc(&iter)) {
    Entity class_e = entity_object(((Class *)key(&iter))->_reflection);
    Entity count_e = entity_int((int)(intptr_t)value(&iter));
    Entity tuple_e = entity_object(tuple_create2(heap, &class_e, &count_e));
    array_add(heap, object_counts, &tuple_e);
  }
  heapprofile_delete(hp);

  // char buffer[32];
  // sprintf(buffer, "%d.csv", tmp++);
  // FILE *file = fopen(buffer, "w");
  // alloc_to_csv(file);
  // fclose(file);

  Entity deleted_nodes_count_e = entity_int(deleted_nodes_count);
  Entity remaining_object_count_e = entity_int(heap_object_count(heap));
  Entity object_counts_e = entity_object(object_counts);
  Object *result_tuple =
      tuple_create3(heap, &deleted_nodes_count_e, &remaining_object_count_e,
                    &object_counts_e);
  return entity_object(result_tuple);
}

Entity _stringify(Task *task, Context *ctx, Object *obj, Entity *args) {
  ASSERT(NOT_NULL(args), PRIMITIVE == args->type);
  Primitive val = args->pri;
  char buffer[BUFFER_SIZE];
  int num_written = 0;
  switch (ptype(&val)) {
  case PRIMITIVE_INT:
    num_written = snprintf(buffer, BUFFER_SIZE, "%" PRId64, pint(&val));
    break;
  case PRIMITIVE_FLOAT:
    num_written = snprintf(buffer, BUFFER_SIZE, "%f", pfloat(&val));
    break;
  default /*CHAR*/:
    num_written = snprintf(buffer, BUFFER_SIZE, "%c", pchar(&val));
    break;
  }
  ASSERT(num_written > 0);
  return entity_object(
      string_new(task->parent_process->heap, buffer, num_written));
}

Entity _string_extend(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (NULL == args || OBJECT != args->type ||
      Class_String != args->obj->_class) {
    return raise_error(task, ctx,
                       "Cannot extend a string with something not a string.");
  }
  String_append((String *)obj->_internal_obj,
                (String *)args->obj->_internal_obj);
  return entity_object(obj);
}

Entity _string_cmp(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (NULL == args || OBJECT != args->type ||
      Class_String != args->obj->_class) {
    return NONE_ENTITY;
  }
  String *self = (String *)obj->_internal_obj;
  String *other = (String *)args->obj->_internal_obj;
  int min_len_cmp = strncmp(self->table, other->table,
                            min(String_size(self), String_size(other)));
  return entity_int((min_len_cmp != 0)
                        ? min_len_cmp
                        : String_size(self) - String_size(other));
}

Entity _string_eq(Task *task, Context *ctx, Object *obj, Entity *args) {
  Primitive p = _string_cmp(task, ctx, obj, args).pri;
  return pint(&p) == 0 ? entity_int(1) : NONE_ENTITY;
}

Entity _string_neq(Task *task, Context *ctx, Object *obj, Entity *args) {
  Primitive p = _string_cmp(task, ctx, obj, args).pri;
  return pint(&p) != 0 ? entity_int(1) : NONE_ENTITY;
}

Entity _string_index(Task *task, Context *ctx, Object *obj, Entity *args) {
  ASSERT(NOT_NULL(args));
  if (PRIMITIVE != args->type || PRIMITIVE_INT != ptype(&args->pri)) {
    return raise_error(task, ctx, "Bad string index input");
  }
  String *self = (String *)obj->_internal_obj;
  int32_t index = pint(&args->pri);
  if (index < 0 || index >= String_size(self)) {
    return raise_error(task, ctx, "Index out of bounds.");
  }
  return entity_char(String_get(self, index));
}

Entity _string_hash(Task *task, Context *ctx, Object *obj, Entity *args) {
  String *str = (String *)obj->_internal_obj;
  return entity_int(string_hasher_len(str->table, String_size(str)));
}

Entity _string_len(Task *task, Context *ctx, Object *obj, Entity *args) {
  String *str = (String *)obj->_internal_obj;
  return entity_int(String_size(str));
}

Entity _string_set(Task *task, Context *ctx, Object *obj, Entity *args) {
  String *str = (String *)obj->_internal_obj;
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
  if (NULL != val && PRIMITIVE == val->type &&
      PRIMITIVE_CHAR == ptype(&val->pri)) {
    String_set(str, pint(&index->pri), pchar(&val->pri));
  } else if (NULL != val && OBJECT == val->type &&
             Class_String == val->obj->_class &&
             1 == String_size(val->obj->_internal_obj)) {
    String_set(str, pint(&index->pri),
               ((String *)val->obj->_internal_obj)->table[0]);
  } else {
    return raise_error(task, ctx, "Bad string index.");
  }
  return NONE_ENTITY;
}

#define IS_OBJECT_CLASS(e, class)                                              \
  ((NULL != (e)) && (OBJECT == (e)->type) && ((class) == (e)->obj->_class))

#define IS_VALUE_TYPE(e, valtype)                                              \
  (((e) != NULL) && (PRIMITIVE == (e)->type) && ((valtype) == ptype(&(e)->pri)))

Entity _string_find(Task *task, Context *ctx, Object *obj, Entity *args) {
  String *str = (String *)obj->_internal_obj;
  if (!IS_TUPLE(args)) {
    return raise_error(task, ctx, "Expected more than one arg.");
  }
  Tuple *tupl_args = (Tuple *)args->obj->_internal_obj;
  if (tuple_size(tupl_args) != 2) {
    return raise_error(task, ctx, "Expected 2 arguments.");
  }
  const Entity *string_arg = tuple_get(tupl_args, 0);
  const Entity *index = tuple_get(tupl_args, 1);
  if (!IS_OBJECT_CLASS(string_arg, Class_String)) {
    return raise_error(task, ctx, "Only a String can be in a String.");
  }
  if (!IS_VALUE_TYPE(index, PRIMITIVE_INT)) {
    return raise_error(task, ctx, "Expected a starting index.");
  }
  String *substr = (String *)string_arg->obj->_internal_obj;

  int32_t index_int = pint(&index->pri);
  if (index_int < 0) {
    return raise_error(task, ctx,
                       "Index out of bounds. Was %d, array length is %d.",
                       index_int, String_size(str));
  }
  if ((index_int + String_size(substr)) > String_size(str)) {
    return NONE_ENTITY;
  }
  char *start_index = str->table + index_int;
  size_t size_after_start = String_size(str) - index_int;

  char *found_index = find_str(start_index, size_after_start, substr->table,
                               String_size(substr));
  if (NULL == found_index) {
    return NONE_ENTITY;
  }
  return entity_int(found_index - start_index);
}

Entity _string_find_all(Task *task, Context *ctx, Object *obj, Entity *args) {
  String *str = (String *)obj->_internal_obj;
  if (!IS_TUPLE(args)) {
    return raise_error(task, ctx, "Expected more than one arg.");
  }
  Tuple *tupl_args = (Tuple *)args->obj->_internal_obj;
  if (tuple_size(tupl_args) != 2) {
    return raise_error(task, ctx, "Expected 2 arguments.");
  }
  const Entity *string_arg = tuple_get(tupl_args, 0);
  const Entity *index = tuple_get(tupl_args, 1);
  if (!IS_OBJECT_CLASS(string_arg, Class_String)) {
    return raise_error(task, ctx, "Only a String can be in a String.");
  }
  if (!IS_VALUE_TYPE(index, PRIMITIVE_INT)) {
    return raise_error(task, ctx, "Expected a starting index.");
  }
  String *substr = (String *)string_arg->obj->_internal_obj;

  int32_t index_int = pint(&index->pri);
  if (index_int < 0) {
    return raise_error(task, ctx,
                       "Index out of bounds. Was %d, array length is %d.",
                       index_int, String_size(str));
  }
  Object *array_obj = array_create(task->parent_process->heap);
  if ((index_int + String_size(substr)) > String_size(str)) {
    return entity_object(array_obj);
  }

  size_t chars_remaining = String_size(str) - index_int;
  char *i_index = str->table + index_int;
  const char *c_substr = substr->table;
  const int substr_len = String_size(substr);
  while (chars_remaining >= substr_len &&
         NULL != (i_index = find_str(i_index, chars_remaining, c_substr,
                                     substr_len))) {
    int index = i_index - str->table;
    Entity index_e = entity_int(index);
    array_add(task->parent_process->heap, array_obj, &index_e);
    i_index++;
    chars_remaining = String_size(str) - index - 1;
  }
  return entity_object(array_obj);
}

Entity _string_substr(Task *task, Context *ctx, Object *obj, Entity *args) {
  String *str = (String *)obj->_internal_obj;

  if (!IS_TUPLE(args)) {
    return raise_error(task, ctx, "Expected more than one arg.");
  }
  Tuple *tupl_args = (Tuple *)args->obj->_internal_obj;
  if (tuple_size(tupl_args) != 2) {
    return raise_error(task, ctx, "Expected 2 arguments.");
  }
  const Entity *index_start = tuple_get(tupl_args, 0);
  if (PRIMITIVE_INT != ptype(&index_start->pri)) {
    return raise_error(task, ctx, "Expected start_index to be Int.");
  }

  const Entity *index_end = tuple_get(tupl_args, 1);
  if (PRIMITIVE_INT != ptype(&index_end->pri)) {
    return raise_error(task, ctx, "Expected end_index to be an Int.");
  }

  int64_t start = pint(&index_start->pri);
  int64_t end = pint(&index_end->pri);

  if (start < 0 || start > String_size(str)) {
    return raise_error(task, ctx, "start_index out of bounds.");
  }
  if (end < 0 || end > String_size(str)) {
    return raise_error(task, ctx, "end_index out of bounds.");
  }
  if (end < start) {
    return raise_error(task, ctx, "start_index > end_index.");
  }
  return entity_object(
      string_new(task->parent_process->heap, str->table + start, end - start));
}

Entity _string_copy(Task *task, Context *ctx, Object *obj, Entity *args) {
  String *str = (String *)obj->_internal_obj;
  return entity_object(
      string_new(task->parent_process->heap, str->table, String_size(str)));
}

Entity _string_ltrim(Task *task, Context *ctx, Object *obj, Entity *args) {
  String *str = (String *)obj->_internal_obj;
  int i = 0;
  while (_is_any_space(str->table[i])) {
    ++i;
  }
  String_lshrink(str, i);
  return entity_object(obj);
}

Entity _string_rtrim(Task *task, Context *ctx, Object *obj, Entity *args) {
  String *str = (String *)obj->_internal_obj;
  int i = 0;
  while (_is_any_space(str->table[String_size(str) - 1 - i]) &&
         i < String_size(str)) {
    ++i;
  }
  String_rshrink(str, i);
  return entity_object(obj);
}

Entity _string_trim(Task *task, Context *ctx, Object *obj, Entity *args) {
  String *str = (String *)obj->_internal_obj;
  int i = 0;
  while (_is_any_space(str->table[i])) {
    ++i;
  }
  String_lshrink(str, i);
  i = 0;
  while (_is_any_space(str->table[String_size(str) - 1 - i])) {
    ++i;
  }
  String_rshrink(str, i);
  return entity_object(obj);
}

Entity _string_lshrink(Task *task, Context *ctx, Object *obj, Entity *args) {
  String *str = (String *)obj->_internal_obj;
  if (NULL == args || PRIMITIVE != args->type ||
      PRIMITIVE_INT != ptype(&args->pri)) {
    return raise_error(task, ctx, "Trimming String with something not an Int.");
  }
  int32_t index = pint(&args->pri);
  if (index > String_size(str)) {
    return raise_error(task, ctx, "Cannot shrink more than the entire size.");
  }
  String_lshrink(str, index);
  return entity_object(obj);
}

Entity _string_rshrink(Task *task, Context *ctx, Object *obj, Entity *args) {
  String *str = (String *)obj->_internal_obj;
  if (NULL == args || PRIMITIVE != args->type ||
      PRIMITIVE_INT != ptype(&args->pri)) {
    return raise_error(task, ctx, "Trimming String with something not an Int.");
  }
  int32_t index = pint(&args->pri);
  if (index > String_size(str)) {
    return raise_error(task, ctx, "Cannot shrink more than the entire size.");
  }
  String_rshrink(str, index);
  return entity_object(obj);
}

Entity _string_clear(Task *task, Context *ctx, Object *obj, Entity *args) {
  String *str = (String *)obj->_internal_obj;
  String_clear(str);
  return entity_object(obj);
}

Entity _string_split(Task *task, Context *ctx, Object *obj, Entity *args) {
  String *str = (String *)obj->_internal_obj;
  if (NULL == args || OBJECT != args->type ||
      Class_String != args->obj->_class) {
    return raise_error(task, ctx,
                       "Argument to String.split() must be a String.");
  }
  Object *array_obj = array_create(task->parent_process->heap);

  int str_len = String_size(str);
  String *delim = (String *)args->obj->_internal_obj;
  int delim_len = String_size(delim);
  int i, last_delim_end = 0;
  for (i = 0; i < str_len; ++i) {
    if (0 == strncmp(str->table + i, delim->table, delim_len)) {
      Entity part = entity_object(string_new(task->parent_process->heap,
                                             str->table + last_delim_end,
                                             i - last_delim_end));
      array_add(task->parent_process->heap, array_obj, &part);
      i += delim_len;
      last_delim_end = i;
    }
  }
  if (last_delim_end < str_len - delim_len) {
    Entity part = entity_object(string_new(task->parent_process->heap,
                                           str->table + last_delim_end,
                                           str_len - last_delim_end));
    array_add(task->parent_process->heap, array_obj, &part);
  }
  return entity_object(array_obj);
}

Entity _string_starts_with(Task *task, Context *ctx, Object *obj,
                           Entity *args) {
  String *str = (String *)obj->_internal_obj;
  String *prefix = (String *)args->obj->_internal_obj;
  size_t lenstr = String_size(str);
  size_t lenprefix = String_size(prefix);
  if (lenprefix > lenstr) {
    return NONE_ENTITY;
  }
  return 0 == strncmp(str->table, prefix->table, lenprefix) ? entity_int(1)
                                                            : NONE_ENTITY;
}

Entity _string_ends_with(Task *task, Context *ctx, Object *obj, Entity *args) {
  String *str = (String *)obj->_internal_obj;
  String *suffix = (String *)args->obj->_internal_obj;
  size_t lenstr = String_size(str);
  size_t lensuffix = String_size(suffix);
  if (lensuffix > lenstr) {
    return NONE_ENTITY;
  }
  return 0 == strncmp(str->table + lenstr - lensuffix, suffix->table, lensuffix)
             ? entity_int(1)
             : NONE_ENTITY;
}

Entity _array_len(Task *task, Context *ctx, Object *obj, Entity *args) {
  Array *arr = (Array *)obj->_internal_obj;
  return entity_int(Array_size(arr));
}

Entity _array_append(Task *task, Context *ctx, Object *obj, Entity *args) {
  array_add(task->parent_process->heap, obj, args);
  return entity_object(obj);
}

Entity _array_remove(Task *task, Context *ctx, Object *obj, Entity *args) {
  return array_remove(task->parent_process->heap, obj, pint(&args->pri));
}

Entity _tuple_len(Task *task, Context *ctx, Object *obj, Entity *args) {
  Tuple *t = (Tuple *)obj->_internal_obj;
  return entity_int(tuple_size(t));
}

Entity _object_class(Task *task, Context *ctx, Object *obj, Entity *args) {
  return entity_object(obj->_class->_reflection);
}

Entity _object_hash(Task *task, Context *ctx, Object *obj, Entity *args) {
  return entity_int((int32_t)(intptr_t)obj);
}

Entity _object_address_int(Task *task, Context *ctx, Object *obj,
                           Entity *args) {
  return entity_int((int32_t)(intptr_t)obj);
}

Entity _object_address_hex(Task *task, Context *ctx, Object *obj,
                           Entity *args) {
  char buffer[BUFFER_SIZE];
  int num_written = snprintf(buffer, BUFFER_SIZE, "%p", obj);
  ASSERT(num_written > 0);
  return entity_object(
      string_new(task->parent_process->heap, buffer, num_written));
}

Entity _class_module(Task *task, Context *ctx, Object *obj, Entity *args) {
  return entity_object(obj->_class_obj->_module->_reflection);
}

Entity _function_name(Task *task, Context *ctx, Object *obj, Entity *args) {
  return entity_object(string_new(task->parent_process->heap,
                                  obj->_function_obj->_name,
                                  strlen(obj->_function_obj->_name)));
}

Entity _function_module(Task *task, Context *ctx, Object *obj, Entity *args) {
  return entity_object(obj->_function_obj->_module->_reflection);
}

Entity _function_is_method(Task *task, Context *ctx, Object *obj,
                           Entity *args) {
  return (NULL == obj->_function_obj->_parent_class) ? NONE_ENTITY
                                                     : entity_int(1);
}
Entity _function_parent_class(Task *task, Context *ctx, Object *obj,
                              Entity *args) {
  return (NULL == obj->_function_obj->_parent_class)
             ? NONE_ENTITY
             : entity_object(obj->_function_obj->_parent_class->_reflection);
}

Entity _class_name(Task *task, Context *ctx, Object *obj, Entity *args) {
  return entity_object(string_new(task->parent_process->heap,
                                  obj->_class_obj->_name,
                                  strlen(obj->_class_obj->_name)));
}

Entity _module_name(Task *task, Context *ctx, Object *obj, Entity *args) {
  return entity_object(string_new(task->parent_process->heap,
                                  obj->_module_obj->_name,
                                  strlen(obj->_module_obj->_name)));
}

Entity _function_ref_name(Task *task, Context *ctx, Object *obj, Entity *args) {
  const char *name = function_ref_get_func(obj)->_name;
  return entity_object(
      string_new(task->parent_process->heap, name, strlen(name)));
}

Entity _function_ref_module(Task *task, Context *ctx, Object *obj,
                            Entity *args) {
  const Function *f = function_ref_get_func(obj);
  return entity_object(f->_module->_reflection);
}

Entity _function_ref_func(Task *task, Context *ctx, Object *obj, Entity *args) {
  const Function *f = function_ref_get_func(obj);
  return entity_object(f->_reflection);
}

Entity _function_ref_obj(Task *task, Context *ctx, Object *obj, Entity *args) {
  Object *f_obj = function_ref_get_object(obj);
  return entity_object(f_obj);
}

void _range_init(Object *obj) { obj->_internal_obj = ALLOC2(_Range); }
void _range_delete(Object *obj) { DEALLOC(obj->_internal_obj); }

Entity _range_constructor(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (NULL == args || OBJECT != args->type ||
      Class_Tuple != args->obj->_class) {
    return raise_error(task, ctx, "Input to range() is not a tuple.");
  }
  Tuple *t = (Tuple *)args->obj->_internal_obj;
  if (3 != tuple_size(t)) {
    return raise_error(task, ctx, "Invalid tuple size for range(). Was %d",
                       tuple_size(t));
  }
  const Entity *first = tuple_get(t, 0);
  const Entity *second = tuple_get(t, 1);
  const Entity *third = tuple_get(t, 2);
  if (PRIMITIVE != first->type || PRIMITIVE_INT != ptype(&first->pri)) {
    return raise_error(task, ctx, "Input to range() is invalid.");
  }
  if (PRIMITIVE != first->type || PRIMITIVE_INT != ptype(&second->pri)) {
    return raise_error(task, ctx, "Input to range() is invalid.");
  }
  if (PRIMITIVE != first->type || PRIMITIVE_INT != ptype(&third->pri)) {
    return raise_error(task, ctx, "Input to range() is invalid.");
  }
  _Range *range = (_Range *)obj->_internal_obj;
  range->start = pint(&first->pri);
  range->inc = pint(&second->pri);
  range->end = pint(&third->pri);
  return entity_object(obj);
}

Entity _range_start(Task *task, Context *ctx, Object *obj, Entity *args) {
  return entity_int(((_Range *)obj->_internal_obj)->start);
}

Entity _range_inc(Task *task, Context *ctx, Object *obj, Entity *args) {
  return entity_int(((_Range *)obj->_internal_obj)->inc);
}

Entity _range_end(Task *task, Context *ctx, Object *obj, Entity *args) {
  return entity_int(((_Range *)obj->_internal_obj)->end);
}

Entity _class_super(Task *task, Context *ctx, Object *obj, Entity *args) {
  return (NULL == obj->_class_obj->_super)
             ? NONE_ENTITY
             : entity_object(obj->_class_obj->_super->_reflection);
}

Entity _object_super(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (!IS_CLASS(args, Class_Class)) {
    return raise_error(task, ctx, "super() requires a Class as an argument.");
  }
  const Class *target_super = args->obj->_class_obj;

  const Class *super = obj->_class->_super;
  const Function *constructor = NULL;
  while (NULL != super) {
    if (super == target_super) {
      constructor = class_get_function(super, CONSTRUCTOR_KEY);
      break;
    }
    super = super->_super;
  }
  if (NULL != constructor) {
    return entity_object(_wrap_function_in_ref2(constructor, obj, task, ctx));
  }
  return NONE_ENTITY;
}

Entity _object_copy(Task *task, Context *ctx, Object *obj, Entity *args) {
  Entity e = entity_object(obj);
  Map cpy_map;
  map_init_default(&cpy_map);
  Entity to_return = entity_copy(task->parent_process->heap, &cpy_map, &e);
  map_finalize(&cpy_map);
  return to_return;
}

Entity _class_methods(Task *task, Context *ctx, Object *obj, Entity *args) {
  ASSERT(obj->_class == Class_Class);
  Object *array_obj = array_create(task->parent_process->heap);
  Class *c = obj->_class_obj;
  KL_iter funcs = class_functions(c);
  for (; kl_has(&funcs); kl_inc(&funcs)) {
    const Function *f = (const Function *)kl_value(&funcs);
    Entity func = entity_object(f->_reflection);
    array_add(task->parent_process->heap, array_obj, &func);
  }
  return entity_object(array_obj);
}

Entity _class_set_super(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (!IS_CLASS(args, Class_Class)) {
    return raise_error(task, ctx,
                       "Argument 1 of $__set_super must be of type Class.");
  }
  Class *class = obj->_class_obj;
  Class *new_super = args->obj->_class_obj;
  class->_super = new_super;
  return entity_object(obj);
}

Entity _class_fields(Task *task, Context *ctx, Object *obj, Entity *args) {
  ASSERT(obj->_class == Class_Class);
  return *object_get(obj, FIELDS_PRIVATE_KEY);
}

Entity _set_member(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (!IS_TUPLE(args)) {
    return raise_error(task, ctx, "$set() can only be called with a Tuple.");
  }
  Tuple *t_args = (Tuple *)args->obj->_internal_obj;
  if (2 != tuple_size(t_args)) {
    return raise_error(task, ctx,
                       "$set() can only be called with 2 args. %d provided.",
                       tuple_size(t_args));
  }
  if (!IS_CLASS(tuple_get(t_args, 0), Class_String)) {
    return raise_error(task, ctx, "First argument to $set() must be a String.");
  }
  const String *str_key =
      (const String *)tuple_get(t_args, 0)->obj->_internal_obj;
  const char *key = intern_range(str_key->table, 0, String_size(str_key));
  object_set_member(task->parent_process->heap, obj, key, tuple_get(t_args, 1));
  return entity_object(obj);
}

Entity _class_set_method(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (!IS_TUPLE(args)) {
    return raise_error(task, ctx,
                       "$set_method() can only be called with a Tuple.");
  }
  Tuple *t_args = (Tuple *)args->obj->_internal_obj;
  if (2 != tuple_size(t_args)) {
    return raise_error(
        task, ctx, "$set_method() can only be called with 2 args. %d provided.",
        tuple_size(t_args));
  }
  if (!IS_CLASS(tuple_get(t_args, 0), Class_String)) {
    return raise_error(task, ctx,
                       "First argument to $set_method() must be a String.");
  }
  const String *str_key =
      (const String *)tuple_get(t_args, 0)->obj->_internal_obj;
  const char *key = intern_range(str_key->table, 0, String_size(str_key));
  const Entity *arg1 = tuple_get(t_args, 1);
  // if (IS_CLASS(arg1, Class_FunctionRef)) {
  //   const Function *func = function_ref_get_func(arg1->obj);
  //   add_reflection_to_function(
  //       task->parent_process->heap, obj,
  //       class_add_function(obj->_class_obj, key, func->_ins_pos,
  //                          func->_is_const, func->_is_async));
  // } else {
  //   return raise_error(
  //       task, ctx, "Second argument to $set_method() must be a
  //       FunctionRef.");
  // }
  object_set_member(task->parent_process->heap, obj, key, arg1);

  return entity_object(obj);
}

Entity _get_member(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (!IS_CLASS(args, Class_String)) {
    return raise_error(task, ctx, "$get() can only be called with a String.");
  }
  const String *str_key = (const String *)args->obj->_internal_obj;
  const char *key = intern_range(str_key->table, 0, String_size(str_key));
  return object_get_maybe_wrap(obj, key, task->parent_process->heap, ctx);
}

void _process_init(Object *obj) {}
void _process_delete(Object *obj) {}

// Not safe after new process is started.
Entity _create_future_for_process(Process *process, Process *new_process,
                                  Task *task) {
  Task *new_task = process_create_unqueued_task(process);
  new_task->parent_task = task;

  Object *future_obj = future_create(new_task);
  new_process->future = (Future *)future_obj->_internal_obj;

  new_task->state = TASK_WAITING;
  process_insert_waiting_task(process, new_task);

  return entity_object(future_obj);
}

Entity _process_start(Task *current_task, Context *current_ctx, Object *obj,
                      Entity *args) {
  Process *process = (Process *)obj->_internal_obj;
  Entity future = _create_future_for_process(current_task->parent_process,
                                             process, current_task);
  process_run_in_new_thread(process);
  return future;
}

Entity _process_send(Task *current_task, Context *current_ctx, Object *obj,
                     Entity *args) {
  Process *process = (Process *)obj->_internal_obj;

  Entity copy;
  if (NULL == args || NONE == args->type) {
    copy = NONE_ENTITY;
  } else {
    Map cps;
    map_init_default(&cps);
    SYNCHRONIZED(process->heap_access_lock,
                 { copy = entity_copy(process->heap, &cps, args); });
    map_finalize(&cps);
  }

  Entity future = NONE_ENTITY;

  return future;
}

void _task_init(Object *obj) {}
void _task_delete(Object *obj) {}

void _context_init(Object *obj) {}
void _context_delete(Object *obj) {
  Context *ctx = (Context *)obj->_internal_obj;
  context_finalize(ctx);
  // This can segfault if the process is cleaned up before the context.
  // This segfault should only occur during DEBUG mode.
  __arena_dealloc(&ctx->parent_task->parent_process->context_arena, ctx);
}

void _remote_init(Object *obj) { create_remote_on_object(obj); }

void _remote_delete(Object *obj) {
  remote_delete(extract_remote_from_obj(obj));
}

Entity _remote_process(Task *current_task, Context *current_ctx, Object *obj,
                       Entity *args) {
  Remote *remote = extract_remote_from_obj(obj);
  return entity_object(remote_get_process(remote)->_reflection);
}

void _builtin_add_string(Module *builtin) {
  native_method(Class_String, intern("extend"), _string_extend);
  native_method(Class_String, CMP_FN_NAME, _string_cmp);
  native_method(Class_String, EQ_FN_NAME, _string_eq);
  native_method(Class_String, NEQ_FN_NAME, _string_neq);
  native_method(Class_String, ARRAYLIKE_INDEX_KEY, _string_index);
  native_method(Class_String, ARRAYLIKE_SET_KEY, _string_set);
  native_method(Class_String, intern("__find"), _string_find);
  native_method(Class_String, intern("__find_all"), _string_find_all);
  native_method(Class_String, intern("len"), _string_len);
  native_method(Class_String, HASH_KEY, _string_hash);
  native_method(Class_String, intern("__substr"), _string_substr);
  native_method(Class_String, intern("copy"), _string_copy);
  native_method(Class_String, intern("ltrim"), _string_ltrim);
  native_method(Class_String, intern("rtrim"), _string_rtrim);
  native_method(Class_String, intern("trim"), _string_trim);
  native_method(Class_String, intern("lshrink"), _string_lshrink);
  native_method(Class_String, intern("rshrink"), _string_rshrink);
  native_method(Class_String, intern("split"), _string_split);
  native_method(Class_String, intern("__starts_with"), _string_starts_with);
  native_method(Class_String, intern("__ends_with"), _string_ends_with);
}

void _builtin_add_function(Module *builtin) {
  native_method(Class_Function, MODULE_KEY, _function_module);
  native_method(Class_Function, PARENT_CLASS, _function_parent_class);
  native_method(Class_Function, intern("is_method"), _function_is_method);
  native_method(Class_FunctionRef, MODULE_KEY, _function_ref_module);
  native_method(Class_Function, NAME_KEY, _function_name);
  native_method(Class_FunctionRef, NAME_KEY, _function_ref_name);
  native_method(Class_FunctionRef, OBJ_KEY, _function_ref_obj);
  native_method(Class_FunctionRef, intern("func"), _function_ref_func);
}

void _builtin_add_range(Module *builtin) {
  Class_Range =
      native_class(builtin, RANGE_CLASS_NAME, _range_init, _range_delete);
  native_method(Class_Range, CONSTRUCTOR_KEY, _range_constructor);
  native_method(Class_Range, intern("start"), _range_start);
  native_method(Class_Range, intern("inc"), _range_inc);
  native_method(Class_Range, intern("end"), _range_end);
}

void _builtin_add_process(Module *builtin) {
  Class_Process =
      native_class(builtin, PROCESS_NAME, _process_init, _process_delete);
  native_method(Class_Process, intern("start"), _process_start);
  Class_Task = native_class(builtin, TASK_NAME, _task_init, _task_delete);
  Class_Context =
      native_class(builtin, CONTEXT_NAME, _context_init, _context_delete);
  Class_Remote =
      native_class(builtin, REMOTE_CLASS_NAME, _remote_init, _remote_delete);
  native_method(Class_Remote, intern("process"), _remote_process);
}

void builtin_add_native(ModuleManager *mm, Module *builtin) {
  builtin_classes(mm->_heap, builtin);

  _builtin_add_process(builtin);

  native_function(builtin, intern("__collect_garbage"), _collect_garbage);
  native_function(builtin, intern("Int"), _Int);
  native_function(builtin, intern("Float"), _Float);
  native_function(builtin, intern("Bool"), __Bool);
  native_function(builtin, intern("__stringify"), _stringify);

  _builtin_add_string(builtin);
  _builtin_add_function(builtin);
  _builtin_add_range(builtin);

  native_method(Class_Class, MODULE_KEY, _class_module);
  native_method(Class_Class, NAME_KEY, _class_name);
  native_method(Class_Class, SUPER_KEY, _class_super);
  native_method(Class_Class, intern("methods"), _class_methods);

  native_method(Class_Class, intern("$__set_super"), _class_set_super);
  native_method(Class_Class, intern("$set_method"), _class_set_method);
  native_method(Class_Class, FIELDS_KEY, _class_fields);

  native_method(Class_Object, CLASS_KEY, _object_class);
  native_method(Class_Object, SUPER_KEY, _object_super);
  native_method(Class_Object, HASH_KEY, _object_hash);
  native_method(Class_Object, intern("copy"), _object_copy);
  native_method(Class_Object, intern("$set"), _set_member);
  native_method(Class_Object, intern("$get"), _get_member);
  native_method(Class_Object, ADDRESS_INT_KEY, _object_address_int);
  native_method(Class_Object, ADDRESS_HEX_KEY, _object_address_hex);

  native_method(Class_Array, intern("len"), _array_len);
  native_method(Class_Array, intern("append"), _array_append);
  native_method(Class_Array, intern("__remove"), _array_remove);

  native_method(Class_Tuple, intern("len"), _tuple_len);

  native_method(Class_Module, NAME_KEY, _module_name);
}
