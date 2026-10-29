// builtin.c
//
// Created on: Aug 29, 2020
//     Author: Jeff Manzione

#include "zinnia/entity/native/builtin.h"

#include <inttypes.h>

#include "file-utils/file_info.h"
#include "file-utils/string_utils.h"
#include "zinnia/alloc/alloc.h"
#include "zinnia/entity/array/array.h"
#include "zinnia/entity/class/classes.h"
#include "zinnia/entity/class/classes_def.h"
#include "zinnia/entity/native/async.h"
#include "zinnia/entity/native/builder/function_context.h"
#include "zinnia/entity/native/error.h"
#include "zinnia/entity/native/native.h"
#include "zinnia/entity/native/native_helpers.h"
#include "zinnia/entity/object.h"
#include "zinnia/entity/primitive.h"
#include "zinnia/entity/string/string.h"
#include "zinnia/entity/string/string_helper.h"
#include "zinnia/entity/tuple/tuple.h"
#include "zinnia/entity/value.h"
#include "zinnia/heap/heap.h"
#include "zinnia/util/string_util.h"
#include "zinnia/util/void_array.h"
#include "zinnia/vm/intern.h"
#include "zinnia/vm/isolate/actor_ref.h"
#include "zinnia/vm/isolate/frame.h"
#include "zinnia/vm/isolate/isolate.h"
#include "zinnia/vm/isolate/isolates.h"
#include "zinnia/vm/isolate/scope.h"
#include "zinnia/vm/vm.h"

// From vm/virtual_machine.h
ThreadHandle isolate_run_in_new_thread(Isolate *isolate);

#ifndef min
#define min(x, y) ((x) > (y) ? (y) : (x))
#endif

#define BUFFER_SIZE 256
#define INFER_FROM_STRING 0

bool is_any_space_(const char c) {
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

bool str_to_int64_(String *str, int64_t *result) {
  char *cstr = ALLOC_STRNDUP(str->table, String_size(str));
  char *endptr;
  *result = strtol(cstr, &endptr, INFER_FROM_STRING);
  // Error scenario.
  if (0 == result && endptr - cstr != String_size(str)) {
    RELEASE(cstr);
    return false;
  }
  RELEASE(cstr);
  return true;
}

bool istr_to_int64_(IString *str, int64_t *result) {
  const char *cstr = str->str;
  char *endptr;
  *result = strtol(cstr, &endptr, INFER_FROM_STRING);
  // Error scenario.
  if (0 == result && endptr - cstr != str->len) {
    return false;
  }
  return true;
}

bool value_to_int64_(const Value *e, int64_t *result) {
  if (IS_CLASS(e, Class_String)) {
    return str_to_int64_(e->obj->_internal_obj, result);
  }
  if (IS_CLASS(e, Class_IString)) {
    return istr_to_int64_(e->obj->_internal_obj, result);
  }
  return false;
}

Value Int_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  int64_t result;
  int len;
  char *raw_str;
  if (NULL == args) {
    return value_int(0);
  }
  switch (args->type) {
    case NONE:
      return value_int(0);
    case OBJECT:
      if (!extract_string(args, &raw_str, &len)) {
        return raise_error(frame, ctx, "Cannot convert input to Int.");
      }
      if (!value_to_int64_(args, &result)) {
        return raise_error(frame, ctx, "Cannot convert input '%*s' to Int.",
                           len, raw_str);
      }
      return value_int(result);
    case PRIMITIVE:
      switch (ptype(&args->pri)) {
        case PRIMITIVE_BOOL:
          return value_int(pbool(&args->pri) ? 1 : 0);
        case PRIMITIVE_CHAR:
          return value_int((int64_t)pchar(&args->pri));
        case PRIMITIVE_INT:
          return *args;
        case PRIMITIVE_FLOAT:
          return value_int((int64_t)pfloat(&args->pri));
        default:
          return raise_error(frame, ctx, "Unknown primitive type.");
      }
    default:
      return raise_error(frame, ctx, "Unknown type.");
  }
  return value_int(0);
}

bool str_to_float_(String *str, double *result) {
  char *cstr = ALLOC_STRNDUP(str->table, String_size(str));
  char *endptr;
  *result = strtod(cstr, &endptr);
  // Error scenario.
  if (0 == result && endptr - cstr != String_size(str)) {
    RELEASE(cstr);
    return false;
  }
  RELEASE(cstr);
  return true;
}

bool istr_to_float_(IString *str, double *result) {
  const char *cstr = str->str;
  char *endptr;
  *result = strtod(cstr, &endptr);
  // Error scenario.
  if (0 == result && endptr - cstr != str->len) {
    return false;
  }
  return true;
}

bool value_to_float_(const Value *e, double *result) {
  if (IS_CLASS(e, Class_String)) {
    return str_to_float_(e->obj->_internal_obj, result);
  }
  if (IS_CLASS(e, Class_IString)) {
    return istr_to_float_(e->obj->_internal_obj, result);
  }
  return false;
}

Value Float_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  double result;
  int len;
  char *raw_str;
  if (NULL == args) {
    return value_float(0);
  }
  switch (args->type) {
    case NONE:
      return value_float(0.f);
    case OBJECT:
      if (!extract_string(args, &raw_str, &len)) {
        return raise_error(frame, ctx, "Cannot convert input to Float.");
      }
      if (!value_to_float_(args, &result)) {
        return raise_error(frame, ctx, "Cannot convert input '%*s' to Float.",
                           len, raw_str);
      }
      return value_float(result);
    case PRIMITIVE:
      switch (ptype(&args->pri)) {
        case PRIMITIVE_BOOL:
          return value_float(pbool(&args->pri) ? 1 : 0);
        case PRIMITIVE_CHAR:
          return value_float((double)pchar(&args->pri));
        case PRIMITIVE_INT:
          return value_float((double)pint(&args->pri));
        case PRIMITIVE_FLOAT:
          return *args;
        default:
          return raise_error(frame, ctx, "Unknown primitive type.");
      }
    default:
      return raise_error(frame, ctx, "Unknown type.");
  }
  return value_float(0.f);
}

bool str_to_bool_(String *str, bool *result) {
  char *cstr = ALLOC_STRNDUP(str->table, String_size(str));
  if (0 == strcmp("True", cstr) || 0 == strcmp("true", cstr) ||
      0 == strcmp("T", cstr) || 0 == strcmp("t", cstr)) {
    *result = true;
    RELEASE(cstr);
    return true;
  }
  if (0 == strcmp("False", cstr) || 0 == strcmp("false", cstr) ||
      0 == strcmp("F", cstr) || 0 == strcmp("f", cstr)) {
    *result = false;
    RELEASE(cstr);
    return true;
  }
  RELEASE(cstr);
  return false;
}

bool istr_to_bool_(IString *str, bool *result) {
  const char *cstr = str->str;
  if (0 == strcmp("True", cstr) || 0 == strcmp("true", cstr) ||
      0 == strcmp("T", cstr) || 0 == strcmp("t", cstr)) {
    *result = true;
    return true;
  }
  if (0 == strcmp("False", cstr) || 0 == strcmp("false", cstr) ||
      0 == strcmp("F", cstr) || 0 == strcmp("f", cstr)) {
    *result = false;
    return true;
  }
  return false;
}

bool value_to_bool_(const Value *e, bool *result) {
  if (IS_CLASS(e, Class_String)) {
    return str_to_bool_((String *)e->obj->_internal_obj, result);
  }
  if (IS_CLASS(e, Class_IString)) {
    return istr_to_bool_((IString *)e->obj->_internal_obj, result);
  }
  return false;
}

Value _Bool_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  bool result;
  int len;
  char *raw_str;
  if (NULL == args) {
    return FALSE_VALUE;
  }
  switch (args->type) {
    case NONE:
      return FALSE_VALUE;
    case OBJECT:
      if (!extract_string(args, &raw_str, &len)) {
        return raise_error(frame, ctx, "Cannot convert input to bool Int.");
      }
      if (!value_to_bool_(args, &result)) {
        return raise_error(frame, ctx,
                           "Cannot convert input '%*s' to bool Int.", len,
                           raw_str);
      }
      return result ? TRUE_VALUE : FALSE_VALUE;
    case PRIMITIVE:
      return IS_TRUE(args) ? TRUE_VALUE : FALSE_VALUE;
    default:
      return raise_error(frame, ctx, "Unknown type.");
  }
  return FALSE_VALUE;
}

Object *wrap_function_in_ref2_(const Function *f, Object *obj, Frame *frame,
                               Scope *ctx) {
  Object *fn_ref = heap_new(frame->parent_isolate->heap, Class_FunctionRef);
  function_ref_init__(fn_ref, obj, f, f->_is_anon ? ctx : NULL,
                      frame->parent_isolate->heap);
  return fn_ref;
}

// volatile int tmp = 0;

Value collect_garbage_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  Isolate *isolate = frame->parent_isolate;
  Heap *heap = isolate->heap;
  uint32_t deleted_nodes_count = isolate_collect_garbage(isolate);

  // printf("Tasks:\n\titem_size=%u\n\tcapacity=%u\n\titem_count=%u\n\tsubarena_"
  //        "capacity=%u\n\tsubarena_count=%u\n",
  //        _arena_item_size_(&process->task_arena),
  //        _arena_capacity_(&process->task_arena),
  //        _arena_item_count_(&process->task_arena),
  //        _arena_subarena_capacity_(&process->task_arena),
  //        _arena_subarena_count_(&process->task_arena));

  // printf(
  //     "Contexts:\n\titem_size=%u\n\tcapacity=%u\n\titem_count=%u\n\tsubarena_"
  //     "capacity=%u\n\tsubarena_count=%u\n",
  //     _arena_item_size_(&process->context_arena),
  //     _arena_capacity_(&process->context_arena),
  //     _arena_item_count_(&process->context_arena),
  //     _arena_subarena_capacity_(&process->context_arena),
  //     _arena_subarena_count_(&process->context_arena));

  Object *object_counts = array_create(heap);

  HeapProfile *hp = heap_create_profile(heap);
  ObjectTypeCountMapIterator iter = heapprofile_object_type_counts(hp);
  for (; ObjectTypeCountMap_has_entry(&iter);
       ObjectTypeCountMap_next_entry(&iter)) {
    Value class_e = value_object(ObjectTypeCountMap_key(&iter)->_reflection);
    Value count_e = value_int(*ObjectTypeCountMap_value(&iter));
    Value tuple_e = value_object(tuple_create2(heap, &class_e, &count_e));
    array_add(heap, object_counts, &tuple_e);
  }
  heapprofile_delete(hp);

  // char buffer[32];
  // sprintf(buffer, "%d.csv", tmp++);
  // FILE *file = FILE_FN(buffer, "w");
  // alloc_to_csv(file);
  // fclose(file);

  Value deleted_nodes_count_e = value_int(deleted_nodes_count);
  Value remaining_object_count_e = value_int(heap_object_count(heap));
  Value object_counts_e = value_object(object_counts);
  Object *result_tuple =
      tuple_create3(heap, &deleted_nodes_count_e, &remaining_object_count_e,
                    &object_counts_e);
  return value_object(result_tuple);
}

Value stringify_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  ASSERT(args != NULL);
  ASSERT(PRIMITIVE == args->type || NONE == args->type);
  if (IS_NONE(args)) {
    return value_object(
        string_new(frame->parent_isolate->heap, "None", strlen("None")));
  }

  char buffer[BUFFER_SIZE];
  int num_written = 0;
  Primitive val = args->pri;

  switch (ptype(&val)) {
    case PRIMITIVE_BOOL:
      num_written =
          snprintf(buffer, BUFFER_SIZE, "%s", pbool(&val) ? "True" : "False");
      break;
    case PRIMITIVE_INT:
      num_written = snprintf(buffer, BUFFER_SIZE, "%" PRId64, pint(&val));
      break;
    case PRIMITIVE_FLOAT:
      num_written = snprintf(buffer, BUFFER_SIZE, "%g", pfloat(&val));
      break;
    default /*CHAR*/:
      num_written = snprintf(buffer, BUFFER_SIZE, "%c", pchar(&val));
      break;
  }
  ASSERT(num_written > 0);
  return value_object(
      string_new(frame->parent_isolate->heap, buffer, num_written));
}

Value tuple_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  ASSERT(args != NULL);
  Array *arr = (Array *)args->obj->_internal_obj;
  Object *tup = heap_new(frame->parent_isolate->heap, Class_Tuple);
  tup->_internal_obj = tuple_create(Array_size(arr));
  for (int i = 0; i < Array_size(arr); ++i) {
    const Value *e = Array_get_ref_unchecked(arr, i);
    tuple_set(frame->parent_isolate->heap, tup, i, e);
  }
  return value_object(tup);
}

Value color_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  if (!IS_NONE(args) && !IS_STRING(args) && !IS_INT(args)) {
    return raise_error(
        frame, ctx, "Function 'color' expect Int, String, or None argument.");
  }
  Object *ret;
  if (IS_NONE(args)) {
    ret = string_new(frame->parent_isolate->heap, "\x1b[m", strlen("\x1b[m"));
  } else if (IS_CLASS(args, Class_String)) {
    String *str = (String *)args->obj->_internal_obj;
    char buf[16];
    sprintf(buf, "\x1b[%*sm", (int)String_size(str), str->table);
    ret = string_new(frame->parent_isolate->heap, buf, strlen(buf));
  } else if (IS_CLASS(args, Class_IString)) {
    IString *istr = (IString *)args->obj->_internal_obj;
    char buf[16];
    sprintf(buf, "\x1b[%*sm", istr->len, istr->str);
    ret = string_new(frame->parent_isolate->heap, buf, strlen(buf));
  } else {
    char buf[16];
    sprintf(buf, "\x1b[%" PRId64 "m", pint(&args->pri));
    ret = string_new(frame->parent_isolate->heap, buf, strlen(buf));
  }
  return value_object(ret);
}

Value string_extend_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  if (IS_CLASS(args, Class_String)) {
    String_append((String *)obj->_internal_obj,
                  (String *)args->obj->_internal_obj);
  } else if (IS_CLASS(args, Class_IString)) {
    const IString *istr = (IString *)args->obj->_internal_obj;
    String_append_raw((String *)obj->_internal_obj, istr->str, istr->len);
  } else {
    return raise_error(frame, ctx,
                       "Cannot extend a string with something not a string.");
  }
  return value_object(obj);
}

Value string_cmp_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  if (!IS_STRING(args)) {
    return value_int(1);
  }
  char *self, *other;
  int self_len, other_len;
  extract_string_obj(obj, &self, &self_len);
  extract_string(args, &other, &other_len);

  int min_len_cmp = strncmp(self, other, min(self_len, other_len));
  return value_int((min_len_cmp != 0) ? min_len_cmp : self_len - other_len);
}

Value istring_cmp_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  if (!IS_STRING(args)) {
    return value_int(1);
  }
  char *self, *other;
  int self_len, other_len;
  extract_string_obj(obj, &self, &self_len);
  extract_string(args, &other, &other_len);

  if (self == other) {
    return value_int(0);
  }

  int min_len_cmp = strncmp(self, other, min(self_len, other_len));
  return value_int((min_len_cmp != 0) ? min_len_cmp : self_len - other_len);
}

Value string_eq_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  Primitive p = string_cmp_(frame, ctx, obj, args).pri;
  return pint(&p) == 0 ? TRUE_VALUE : FALSE_VALUE;
}

Value istring_eq_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  Primitive p = istring_cmp_(frame, ctx, obj, args).pri;
  return pint(&p) == 0 ? TRUE_VALUE : FALSE_VALUE;
}

Value string_neq_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  Primitive p = string_cmp_(frame, ctx, obj, args).pri;
  return pint(&p) != 0 ? TRUE_VALUE : FALSE_VALUE;
}

Value istring_neq_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  Primitive p = istring_cmp_(frame, ctx, obj, args).pri;
  return pint(&p) != 0 ? TRUE_VALUE : FALSE_VALUE;
}

Value string_index_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  ASSERT(args != NULL);
  if (PRIMITIVE != args->type || PRIMITIVE_INT != ptype(&args->pri)) {
    return raise_error(frame, ctx, "Bad string index input");
  }
  String *self = (String *)obj->_internal_obj;
  int32_t index = pint(&args->pri);
  if (index < 0 || index >= String_size(self)) {
    return raise_error(frame, ctx, "Index out of bounds.");
  }
  return value_char(String_get_unchecked(self, index));
}

Value istring_index_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  ASSERT(args != NULL);
  if (PRIMITIVE != args->type || PRIMITIVE_INT != ptype(&args->pri)) {
    return raise_error(frame, ctx, "Bad string index input");
  }

  char *self;
  int self_len;
  extract_string_obj(obj, &self, &self_len);

  int32_t index = pint(&args->pri);
  if (index < 0 || index >= self_len) {
    return raise_error(frame, ctx, "Index out of bounds.");
  }
  return value_char(self[index]);
}

Value string_hash_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  String *str = (String *)obj->_internal_obj;
  return value_int(hash_string(str->table, String_size(str)));
}

Value istring_hash_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  // Must match string_hash_() so equal String and IString keys hash the same.
  IString *str = (IString *)obj->_internal_obj;
  return value_int(hash_string(str->str, str->len));
}

Value string_len_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  String *str = (String *)obj->_internal_obj;
  return value_int(String_size(str));
}

Value istring_len_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  IString *istr = (IString *)obj->_internal_obj;
  return value_int(istr->len);
}

Value string_set_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  String *str = (String *)obj->_internal_obj;

  EXTRACT_TUPLE_ARGS(tupl_args, args, 2, frame, ctx);
  EXTRACT_INT_AT_INDEX_OR_THROW(const uint64_t index, tupl_args, 0);
  const Value *val = tuple_get(tupl_args, 1);

  if (NULL != val && PRIMITIVE == val->type &&
      PRIMITIVE_CHAR == ptype(&val->pri)) {
    String_set(str, index, pchar(&val->pri));
  } else if (IS_CLASS(val, Class_String)) {
    String_set(str, index, ((String *)val->obj->_internal_obj)->table[0]);
  } else if (IS_CLASS(val, Class_IString)) {
    String_set(str, index, ((IString *)val->obj->_internal_obj)->str[0]);
  } else {
    return raise_error(frame, ctx, "Bad string index.");
  }
  return NONE_VALUE;
}

#define IS_OBJECT_CLASS(e, class) \
  ((NULL != (e)) && (OBJECT == (e)->type) && ((class) == (e)->obj->_class))

#define IS_VALUE_TYPE(e, valtype) \
  (((e) != NULL) && (PRIMITIVE == (e)->type) && ((valtype) == ptype(&(e)->pri)))

Value string_find_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  String *str = (String *)obj->_internal_obj;
  EXTRACT_TUPLE_ARGS(tupl_args, args, 2, frame, ctx);
  EXCTRACT_STRING_AT_INDEX_OR_THROW(substr, substr_len, tupl_args, 0);
  EXTRACT_INT_AT_INDEX_OR_THROW(const int64_t index, tupl_args, 1);

  if (index < 0) {
    return raise_error(frame, ctx,
                       "Index out of bounds. Was %d, array length is %d.",
                       index, String_size(str));
  }
  if ((index + substr_len) > String_size(str)) {
    return NONE_VALUE;
  }
  char *start_index = str->table + index;
  size_t size_after_start = String_size(str) - index;

  char *found_index =
      find_str(start_index, size_after_start, substr, substr_len);
  if (NULL == found_index) {
    return NONE_VALUE;
  }
  return value_int(found_index - start_index);
}

Value istring_find_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  char *str;
  int str_len;
  extract_string_obj(obj, &str, &str_len);

  EXTRACT_TUPLE_ARGS(tupl_args, args, 2, frame, ctx);
  EXCTRACT_STRING_AT_INDEX_OR_THROW(substr, substr_len, tupl_args, 0);
  EXTRACT_INT_AT_INDEX_OR_THROW(const int64_t index, tupl_args, 1);

  if (index < 0) {
    return raise_error(frame, ctx,
                       "Index out of bounds. Was %d, array length is %d.",
                       index, substr_len);
  }
  if ((index + substr_len) > substr_len) {
    return NONE_VALUE;
  }
  char *start_index = str + index;
  size_t size_after_start = str_len - index;

  char *found_index =
      find_str(start_index, size_after_start, substr, substr_len);
  if (NULL == found_index) {
    return NONE_VALUE;
  }
  return value_int(found_index - start_index);
}

Value string_find_all_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  String *str = (String *)obj->_internal_obj;

  EXTRACT_TUPLE_ARGS(tupl_args, args, 2, frame, ctx);
  EXCTRACT_STRING_AT_INDEX_OR_THROW(substr, substr_len, tupl_args, 0);
  EXTRACT_INT_AT_INDEX_OR_THROW(const int64_t index, tupl_args, 1);

  if (index < 0) {
    return raise_error(frame, ctx,
                       "Index out of bounds. Was %d, array length is %d.",
                       index, String_size(str));
  }
  Object *array_obj = array_create(frame->parent_isolate->heap);
  if ((index + substr_len) > String_size(str)) {
    return value_object(array_obj);
  }

  size_t chars_remaining = String_size(str) - index;
  char *i_index = str->table + index;
  while (chars_remaining >= substr_len &&
         NULL != (i_index =
                      find_str(i_index, chars_remaining, substr, substr_len))) {
    int index = i_index - str->table;
    Value index_e = value_int(index);
    array_add(frame->parent_isolate->heap, array_obj, &index_e);
    i_index++;
    chars_remaining = String_size(str) - index - 1;
  }
  return value_object(array_obj);
}

Value istring_find_all_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  char *str;
  int str_len;
  extract_string_obj(obj, &str, &str_len);

  EXTRACT_TUPLE_ARGS(tupl_args, args, 2, frame, ctx);
  EXCTRACT_STRING_AT_INDEX_OR_THROW(substr, substr_len, tupl_args, 0);
  EXTRACT_INT_AT_INDEX_OR_THROW(const int64_t index, tupl_args, 1);

  if (index < 0) {
    return raise_error(frame, ctx,
                       "Index out of bounds. Was %d, array length is %d.",
                       index, str_len);
  }
  Object *array_obj = array_create(frame->parent_isolate->heap);
  if ((index + substr_len) > str_len) {
    return value_object(array_obj);
  }

  size_t chars_remaining = str_len - index;
  char *i_index = str + index;
  while (chars_remaining >= substr_len &&
         NULL != (i_index =
                      find_str(i_index, chars_remaining, substr, substr_len))) {
    int index = i_index - str;
    Value index_e = value_int(index);
    array_add(frame->parent_isolate->heap, array_obj, &index_e);
    i_index++;
    chars_remaining = str_len - index - 1;
  }
  return value_object(array_obj);
}

Value string_substr_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  String *str = (String *)obj->_internal_obj;

  EXTRACT_TUPLE_ARGS(tupl_args, args, 2, frame, ctx);
  EXTRACT_INT_AT_INDEX_OR_THROW(const int64_t start, tupl_args, 0);
  EXTRACT_INT_AT_INDEX_OR_THROW(const int64_t end, tupl_args, 1);

  if (start < 0 || start > String_size(str)) {
    return raise_error(frame, ctx, "start_index out of bounds.");
  }
  if (end < 0 || end > String_size(str)) {
    return raise_error(frame, ctx, "end_index out of bounds.");
  }
  if (end < start) {
    return raise_error(frame, ctx, "start_index > end_index.");
  }
  return value_object(
      string_new(frame->parent_isolate->heap, str->table + start, end - start));
}

Value istring_substr_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  char *str;
  int str_len;
  extract_string_obj(obj, &str, &str_len);

  EXTRACT_TUPLE_ARGS(tupl_args, args, 2, frame, ctx);
  EXTRACT_INT_AT_INDEX_OR_THROW(const int64_t start, tupl_args, 0);
  EXTRACT_INT_AT_INDEX_OR_THROW(const int64_t end, tupl_args, 1);

  if (start < 0 || start > str_len) {
    return raise_error(frame, ctx, "start_index out of bounds.");
  }
  if (end < 0 || end > str_len) {
    return raise_error(frame, ctx, "end_index out of bounds.");
  }
  if (end < start) {
    return raise_error(frame, ctx, "start_index > end_index.");
  }
  return value_object(
      string_new(frame->parent_isolate->heap, str + start, end - start));
}

Value string_copy_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  String *str = (String *)obj->_internal_obj;
  return value_object(
      string_new(frame->parent_isolate->heap, str->table, String_size(str)));
}

// Returns a mutable String with the same contents.
Value istring_copy_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  IString *istr = (IString *)obj->_internal_obj;
  return value_object(
      string_new(frame->parent_isolate->heap, istr->str, istr->len));
}

// Sets [*start, *end) to the range of str without leading and/or trailing
// whitespace.
static void trim_bounds_(const char *str, int size, bool left, bool right,
                         int *start, int *end) {
  *start = 0;
  *end = size;
  if (left) {
    while (*start < *end && is_any_space_(str[*start])) {
      ++*start;
    }
  }
  if (right) {
    while (*end > *start && is_any_space_(str[*end - 1])) {
      --*end;
    }
  }
}

// Removes leading and/or trailing whitespace from str.
static void trim_(String *str, bool left, bool right) {
  const int size = String_size(str);
  int start, end;
  trim_bounds_(str->table, size, left, right, &start, &end);
  // Shrink right first so the left shift moves only the kept characters.
  // Shrinking by 0 is invalid.
  if (end < size) {
    String_rshrink(str, size - end);
  }
  if (start > 0) {
    String_lshrink(str, start);
  }
}

Value string_ltrim_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  trim_((String *)obj->_internal_obj, /*left=*/true, /*right=*/false);
  return value_object(obj);
}

Value string_rtrim_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  trim_((String *)obj->_internal_obj, /*left=*/false, /*right=*/true);
  return value_object(obj);
}

Value string_trim_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  trim_((String *)obj->_internal_obj, /*left=*/true, /*right=*/true);
  return value_object(obj);
}

// IStrings are immutable, so trimming one returns a new String.
static Value istring_trim_impl_(Frame *frame, Object *obj, bool left,
                                bool right) {
  IString *istr = (IString *)obj->_internal_obj;
  int start, end;
  trim_bounds_(istr->str, istr->len, left, right, &start, &end);
  return value_object(
      string_new(frame->parent_isolate->heap, istr->str + start, end - start));
}

Value istring_ltrim_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  return istring_trim_impl_(frame, obj, /*left=*/true, /*right=*/false);
}

Value istring_rtrim_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  return istring_trim_impl_(frame, obj, /*left=*/false, /*right=*/true);
}

Value istring_trim_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  return istring_trim_impl_(frame, obj, /*left=*/true, /*right=*/true);
}

Value string_lshrink_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  String *str = (String *)obj->_internal_obj;
  if (NULL == args || PRIMITIVE != args->type ||
      PRIMITIVE_INT != ptype(&args->pri)) {
    return raise_error(frame, ctx,
                       "Trimming String with something not an Int.");
  }
  int32_t index = pint(&args->pri);
  if (index < 0) {
    return raise_error(frame, ctx, "Cannot shrink by a negative amount.");
  }
  if (index > String_size(str)) {
    return raise_error(frame, ctx, "Cannot shrink more than the entire size.");
  }
  if (index > 0) {
    String_lshrink(str, index);
  }
  return value_object(obj);
}

Value string_rshrink_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  String *str = (String *)obj->_internal_obj;
  if (NULL == args || PRIMITIVE != args->type ||
      PRIMITIVE_INT != ptype(&args->pri)) {
    return raise_error(frame, ctx,
                       "Trimming String with something not an Int.");
  }
  int32_t index = pint(&args->pri);
  if (index < 0) {
    return raise_error(frame, ctx, "Cannot shrink by a negative amount.");
  }
  if (index > String_size(str)) {
    return raise_error(frame, ctx, "Cannot shrink more than the entire size.");
  }
  if (index > 0) {
    String_rshrink(str, index);
  }
  return value_object(obj);
}

Value string_clear_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  String *str = (String *)obj->_internal_obj;
  String_clear(str);
  return value_object(obj);
}

typedef struct {
  const char *str;
  int len;
  const char *delim;
  int delim_len;
  int pos;
  bool done;
} Splitter_;

// Sets [*start, *start + *len) to the next part of the string. Returns false
// when there are no more parts.
static bool splitter_next_(Splitter_ *s, int *start, int *len) {
  if (s->done) {
    return false;
  }
  *start = s->pos;
  for (int i = s->pos; i + s->delim_len <= s->len; ++i) {
    if (0 == memcmp(s->str + i, s->delim, s->delim_len)) {
      *len = i - s->pos;
      s->pos = i + s->delim_len;
      return true;
    }
  }
  *len = s->len - s->pos;
  s->done = true;
  return true;
}

// Splits str around each occurrence of the delimiter in args. Empty parts are
// kept, except trailing ones (so a trailing delimiter adds no part).
static Value split_(Frame *frame, Scope *ctx, const char *str, int len,
                    const Value *args) {
  if (!IS_STRING(args)) {
    return raise_error(frame, ctx, "Argument to split() must be a String.");
  }
  char *delim;
  int delim_len;
  extract_string(args, &delim, &delim_len);
  if (delim_len == 0) {
    return raise_error(frame, ctx, "Argument to split() must not be empty.");
  }
  // Count the parts to keep, which excludes trailing empty parts.
  Splitter_ splitter = {str, len, delim, delim_len, 0, false};
  int num_parts = 0, num_parts_to_keep = 0, start, part_len;
  while (splitter_next_(&splitter, &start, &part_len)) {
    ++num_parts;
    if (part_len > 0) {
      num_parts_to_keep = num_parts;
    }
  }
  Object *array_obj = array_create(frame->parent_isolate->heap);
  splitter = (Splitter_){str, len, delim, delim_len, 0, false};
  for (int i = 0; i < num_parts_to_keep; ++i) {
    splitter_next_(&splitter, &start, &part_len);
    Value part = value_object(
        string_new(frame->parent_isolate->heap, str + start, part_len));
    array_add(frame->parent_isolate->heap, array_obj, &part);
  }
  return value_object(array_obj);
}

Value string_split_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  String *str = (String *)obj->_internal_obj;
  return split_(frame, ctx, str->table, String_size(str), args);
}

Value istring_split_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  IString *istr = (IString *)obj->_internal_obj;
  return split_(frame, ctx, istr->str, istr->len, args);
}

Value string_starts_with_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  String *str = (String *)obj->_internal_obj;
  size_t lenstr = String_size(str);

  char *prefix;
  int lenprefix;
  extract_string(args, &prefix, &lenprefix);

  if (lenprefix > lenstr) {
    return NONE_VALUE;
  }
  return 0 == strncmp(str->table, prefix, lenprefix) ? TRUE_VALUE : FALSE_VALUE;
}

Value istring_starts_with_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  char *str;
  int lenstr;
  extract_string_obj(obj, &str, &lenstr);

  char *prefix;
  int lenprefix;
  extract_string(args, &prefix, &lenprefix);

  if (lenprefix > lenstr) {
    return NONE_VALUE;
  }
  return 0 == strncmp(str, prefix, lenprefix) ? TRUE_VALUE : FALSE_VALUE;
}

Value string_ends_with_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  String *str = (String *)obj->_internal_obj;
  size_t lenstr = String_size(str);

  char *suffix;
  int lensuffix;
  extract_string(args, &suffix, &lensuffix);

  if (lensuffix > lenstr) {
    return NONE_VALUE;
  }
  return 0 == strncmp(str->table + lenstr - lensuffix, suffix, lensuffix)
             ? TRUE_VALUE
             : FALSE_VALUE;
}

Value istring_ends_with_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  char *str;
  int lenstr;
  extract_string_obj(obj, &str, &lenstr);

  char *suffix;
  int lensuffix;
  extract_string(args, &suffix, &lensuffix);

  if (lensuffix > lenstr) {
    return NONE_VALUE;
  }
  return 0 == strncmp(str + lenstr - lensuffix, suffix, lensuffix)
             ? TRUE_VALUE
             : FALSE_VALUE;
}

Value istring_to_s_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  char *str;
  int len;
  extract_string_obj(obj, &str, &len);
  return value_object(string_new(frame->parent_isolate->heap, str, len));
}

Value array_len_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  Array *arr = (Array *)obj->_internal_obj;
  return value_int(Array_size(arr));
}

Value array_append_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  array_add(frame->parent_isolate->heap, obj, args);
  return value_object(obj);
}

Value array_remove_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  return array_remove(frame->parent_isolate->heap, obj, pint(&args->pri));
}

Value tuple_len_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  Tuple *t = (Tuple *)obj->_internal_obj;
  return value_int(tuple_size(t));
}

Value object_class_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  return value_object(obj->_class->_reflection);
}

Value object_hash_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  return value_int((int32_t)(intptr_t)obj);
}

Value object_address_int_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  return value_int((int32_t)(intptr_t)obj);
}

Value object_address_hex_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  char buffer[BUFFER_SIZE];
  int num_written = snprintf(buffer, BUFFER_SIZE, "%p", obj);
  ASSERT(num_written > 0);
  return value_object(
      string_new(frame->parent_isolate->heap, buffer, num_written));
}

Value class_module_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  return value_object(obj->_class_obj->_module->_reflection);
}

Value function_name_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  return value_object(istring_new_no_intern(frame->parent_isolate->heap,
                                            obj->_function_obj->_name,
                                            strlen(obj->_function_obj->_name)));
}

Value function_module_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  return value_object(obj->_function_obj->_module->_reflection);
}

Value function_is_method_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  return (NULL == obj->_function_obj->_parent_class) ? FALSE_VALUE : TRUE_VALUE;
}
Value function_parent_class_(Frame *frame, Scope *ctx, Object *obj,
                             Value *args) {
  return (NULL == obj->_function_obj->_parent_class)
             ? NONE_VALUE
             : value_object(obj->_function_obj->_parent_class->_reflection);
}

Value class_name_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  return value_object(istring_new_no_intern(frame->parent_isolate->heap,
                                            obj->_class_obj->_name,
                                            strlen(obj->_class_obj->_name)));
}

Value module_name_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  return value_object(istring_new_no_intern(frame->parent_isolate->heap,
                                            obj->_module_obj->_name,
                                            strlen(obj->_module_obj->_name)));
}

Value module_functions_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  ASSERT(obj->_class == Class_Module);
  Object *array_obj = array_create(frame->parent_isolate->heap);
  Module *m = obj->_module_obj;
  FunctionMapIterator funcs = module_functions(m);
  for (; FunctionMap_has_entry(&funcs); FunctionMap_next_entry(&funcs)) {
    const Function *f = FunctionMap_value(&funcs);
    Value func = value_object(f->_reflection);
    array_add(frame->parent_isolate->heap, array_obj, &func);
  }
  return value_object(array_obj);
}

Value module_classes_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  ASSERT(obj->_class == Class_Module);
  Object *array_obj = array_create(frame->parent_isolate->heap);
  Module *m = obj->_module_obj;
  ClassMapIterator classes = module_classes(m);
  for (; ClassMap_has_entry(&classes); ClassMap_next_entry(&classes)) {
    const Class *c = ClassMap_value(&classes);
    Value class = value_object(c->_reflection);
    array_add(frame->parent_isolate->heap, array_obj, &class);
  }
  return value_object(array_obj);
}

Value module_has_dl_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  ASSERT(obj->_class == Class_Module);
  Module *m = obj->_module_obj;
  return m->dl != NULL ? TRUE_VALUE : FALSE_VALUE;
}

Value module_dlcall_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  ASSERT(obj->_class == Class_Module);
  Module *m = obj->_module_obj;
  if (!m->dl) {
    return raise_error(
        frame, ctx,
        "Attempted dlcall on module without dynamically-linked library.");
  }
  if (!IS_STRING(args) && !IS_TUPLE(args)) {
    return raise_error(frame, ctx,
                       "Attempted dlcall on module with invalid arguments.");
  }
  const char *fn_name;
  Value args_to_pass = NONE_VALUE;
  if (IS_STRING(args)) {
    fn_name = value_string_copy(args);
  } else {
    Tuple *tuple = (Tuple *)args->obj->_internal_obj;
    if (tuple_size(tuple) == 0) {
      return raise_error(frame, ctx,
                         "Attempted dlcall on module with invalid arguments.");
    }
    fn_name = value_string_copy(tuple_get(tuple, 0));

    if (tuple_size(tuple) == 2) {
      args_to_pass = *tuple_get(tuple, 1);
    } else {
      Object *tuple_obj = heap_new(frame->parent_isolate->heap, Class_Tuple);
      tuple_obj->_internal_obj = tuple_create(tuple_size(tuple) - 1);
      args_to_pass = value_object(tuple_obj);
      for (int i = 0; i < tuple_size(tuple) - 1; ++i) {
        tuple_set(frame->parent_isolate->heap, tuple_obj, i,
                  tuple_get(tuple, i + 1));
      }
    }
  }
  char error_buf[255];
  NativeFunctionHandlerFn fn;
  if (!open_dl_sym(m->dl, fn_name, (DlFnHandle *)&fn, error_buf)) {
    Value error = raise_error(
        frame, ctx,
        "Attempted dlcall on module which did not have function by name '%s'.",
        fn_name);
    RELEASE(fn_name);
    return error;
  }
  FunctionContext fn_ctx;
  FunctionContext_init(&fn_ctx, frame, ctx, &args_to_pass);
  fn(&fn_ctx);
  RELEASE(fn_name);
  return *FunctionContext_get_retval(&fn_ctx);
}

Value function_ref_name_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  const char *name = function_ref_get_func(obj)->_name;
  return value_object(
      istring_new_no_intern(frame->parent_isolate->heap, name, strlen(name)));
}

Value function_ref_module_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  const Function *f = function_ref_get_func(obj);
  return value_object(f->_module->_reflection);
}

Value function_ref_func_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  const Function *f = function_ref_get_func(obj);
  return value_object(f->_reflection);
}

Value function_ref_obj_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  Object *f_obj = function_ref_get_object(obj);
  return value_object(f_obj);
}

void range_init_(Object *obj) { obj->_internal_obj = CNEW(Range_); }
void range_delete_(Object *obj) { RELEASE(obj->_internal_obj); }

void range_copy_(ValueCopier *copier, Object *src_obj, Object *target_obj) {
  Range_ *src = (Range_ *)src_obj->_internal_obj;
  Range_ *target = (Range_ *)target_obj->_internal_obj;
  *target = *src;
}

Value range_constructor_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  EXTRACT_TUPLE_ARGS(t, args, 3, frame, ctx);

  Range_ *range = (Range_ *)obj->_internal_obj;

  EXTRACT_INT_AT_INDEX_OR_THROW(range->start, t, 0);
  EXTRACT_INT_AT_INDEX_OR_THROW(range->inc, t, 1);
  EXTRACT_INT_AT_INDEX_OR_THROW(range->end, t, 2);

  return value_object(obj);
}

Value range_start_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  return value_int(((Range_ *)obj->_internal_obj)->start);
}

Value range_inc_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  return value_int(((Range_ *)obj->_internal_obj)->inc);
}

Value range_end_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  return value_int(((Range_ *)obj->_internal_obj)->end);
}

Value class_super_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  return (NULL == obj->_class_obj->_super)
             ? NONE_VALUE
             : value_object(obj->_class_obj->_super->_reflection);
}

Value class_is_subclass_of_(Frame *frame, Scope *ctx, Object *obj,
                            Value *args) {
  if (!IS_CLASS(args, Class_Class)) {
    return raise_error(frame, ctx,
                       "is_subclass_of() requires a Class as an argument.");
  }
  const bool is_subclass_of =
      inherits_from(obj->_class_obj, args->obj->_class_obj);
  return is_subclass_of ? TRUE_VALUE : FALSE_VALUE;
}

Value object_super_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  if (!IS_CLASS(args, Class_Class)) {
    return raise_error(frame, ctx, "super() requires a Class as an argument.");
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
    return value_object(wrap_function_in_ref2_(constructor, obj, frame, ctx));
  }
  return NONE_VALUE;
}

Value object_copy_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  Value e = value_object(obj);
  return value_copy(&e, frame->parent_isolate->heap);
}

Value class_methods_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  ASSERT(obj->_class == Class_Class);
  Object *array_obj = array_create(frame->parent_isolate->heap);
  Class *c = obj->_class_obj;
  FunctionMapIterator funcs = class_functions(c);
  for (; FunctionMap_has_entry(&funcs); FunctionMap_next_entry(&funcs)) {
    const Function *f = FunctionMap_value(&funcs);
    Value func = value_object(f->_reflection);
    array_add(frame->parent_isolate->heap, array_obj, &func);
  }
  return value_object(array_obj);
}

Value class_set_super_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  if (!IS_CLASS(args, Class_Class)) {
    return raise_error(frame, ctx,
                       "Argument 1 of $_set_super_ must be of type Class.");
  }
  Class *class = obj->_class_obj;
  Class *new_super = args->obj->_class_obj;
  class->_super = new_super;
  return value_object(obj);
}

Value class_fields_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  ASSERT(obj->_class == Class_Class);
  return *object_get(obj, FIELDS_PRIVATE_KEY);
}

Value set_member_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  EXTRACT_TUPLE_ARGS(t_args, args, 2, frame, ctx);

  if (!IS_STRING(tuple_get(t_args, 0))) {
    return raise_error(frame, ctx,
                       "First argument to $set() must be a String.");
  }
  const char *key = intern_value(tuple_get(t_args, 0));
  object_set_member(frame->parent_isolate->heap, obj, key,
                    tuple_get(t_args, 1));
  return value_object(obj);
}

Value class_set_method_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  EXTRACT_TUPLE_ARGS(t_args, args, 2, frame, ctx);

  if (!IS_STRING(tuple_get(t_args, 0))) {
    return raise_error(frame, ctx,
                       "First argument to $set_method() must be a String.");
  }
  const char *key = intern_value(tuple_get(t_args, 0));
  const Value *arg1 = tuple_get(t_args, 1);
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
  object_set_member(frame->parent_isolate->heap, obj, key, arg1);

  return value_object(obj);
}

Value get_member_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  if (!IS_STRING(args)) {
    return raise_error(frame, ctx, "$get() can only be called with a String.");
  }
  const char *key = intern_value(args);
  return object_get_maybe_wrap(obj, key, frame->parent_isolate->heap, ctx);
}

void isolate_init_(Object *obj) {}
void isolate_delete_(Object *obj) {}

// Not safe after new process is started.
Value create_future_for_isolate_(Isolate *isolate, Isolate *new_isolate,
                                 Frame *frame) {
  Frame *new_frame = isolate_create_unqueued_frame(isolate);
  new_frame->parent_frame = frame;

  Object *future_obj = future_create(new_frame);
  new_isolate->future = (Future *)future_obj->_internal_obj;

  isolate_insert_waiting_frame(isolate, new_frame);

  return value_object(future_obj);
}

Value isolate_start_(Frame *current_frame, Scope *current_ctx, Object *obj,
                     Value *args) {
  Isolate *isolate = (Isolate *)obj->_internal_obj;
  Value future = create_future_for_isolate_(current_frame->parent_isolate,
                                            isolate, current_frame);
  isolate_run_in_new_thread(isolate);
  return future;
}

void frame_init_(Object *obj) {}
void frame_delete_(Object *obj) {}

void scope_init_(Object *obj) {}
void scope_delete_(Object *obj) {
  Scope *ctx = (Scope *)obj->_internal_obj;
  scope_finalize(ctx);
  // This can segfault if the process is cleaned up before the context.
  // This segfault should only occur during DEBUG mode.
  arena_free(&ctx->parent_frame->parent_isolate->scope_arena, ctx);
}

void actor_init_(Object *obj) { create_actor_on_object(obj); }

void actor_delete_(Object *obj) { actor_delete(extract_actor_from_obj(obj)); }

// Copies refer to the same actor, so ActorRefs can be passed to other isolates.
void actor_copy_(ValueCopier *copier, Object *src_obj, Object *target_obj) {
  ActorRef *src = extract_actor_from_obj(src_obj);
  actor_init_ref(extract_actor_from_obj(target_obj), actor_get_isolate(src),
                 actor_get_object(src));
}

Value actor_isolate_(Frame *current_frame, Scope *current_ctx, Object *obj,
                     Value *args) {
  ActorRef *actor = extract_actor_from_obj(obj);
  return value_object(actor_get_isolate(actor)->_reflection);
}

void builtin_add_string_(Module *builtin) {
  native_method(Class_String, global_intern("extend"), string_extend_);
  native_method(Class_String, CMP_FN_NAME, string_cmp_);
  native_method(Class_String, EQ_FN_NAME, string_eq_);
  native_method(Class_String, NEQ_FN_NAME, string_neq_);
  native_method(Class_String, ARRAYLIKE_INDEX_KEY, string_index_);
  native_method(Class_String, ARRAYLIKE_SET_KEY, string_set_);
  native_method(Class_String, global_intern("__find"), string_find_);
  native_method(Class_String, global_intern("__find_all"), string_find_all_);
  native_method(Class_String, global_intern("len"), string_len_);
  native_method(Class_String, HASH_KEY, string_hash_);
  native_method(Class_String, global_intern("__substr"), string_substr_);
  native_method(Class_String, global_intern("copy"), string_copy_);
  native_method(Class_String, global_intern("ltrim"), string_ltrim_);
  native_method(Class_String, global_intern("rtrim"), string_rtrim_);
  native_method(Class_String, global_intern("trim"), string_trim_);
  native_method(Class_String, global_intern("lshrink"), string_lshrink_);
  native_method(Class_String, global_intern("rshrink"), string_rshrink_);
  native_method(Class_String, global_intern("split"), string_split_);
  native_method(Class_String, global_intern("__starts_with"),
                string_starts_with_);
  native_method(Class_String, global_intern("__ends_with"), string_ends_with_);
}

void builtin_add_istring_(Module *builtin) {
  native_method(Class_IString, CMP_FN_NAME, istring_cmp_);
  native_method(Class_IString, EQ_FN_NAME, istring_eq_);
  native_method(Class_IString, NEQ_FN_NAME, istring_neq_);
  native_method(Class_IString, ARRAYLIKE_INDEX_KEY, istring_index_);
  native_method(Class_IString, global_intern("__find"), istring_find_);
  native_method(Class_IString, global_intern("__find_all"), istring_find_all_);
  native_method(Class_IString, global_intern("len"), istring_len_);
  native_method(Class_IString, HASH_KEY, istring_hash_);
  native_method(Class_IString, global_intern("__substr"), istring_substr_);
  native_method(Class_IString, global_intern("copy"), istring_copy_);
  native_method(Class_IString, global_intern("ltrim"), istring_ltrim_);
  native_method(Class_IString, global_intern("rtrim"), istring_rtrim_);
  native_method(Class_IString, global_intern("trim"), istring_trim_);
  native_method(Class_IString, global_intern("split"), istring_split_);
  native_method(Class_IString, global_intern("__starts_with"),
                istring_starts_with_);
  native_method(Class_IString, global_intern("__ends_with"),
                istring_ends_with_);
  native_method(Class_IString, TO_S_KEY, istring_to_s_);
}

void builtin_add_function_(Module *builtin) {
  native_method(Class_Function, MODULE_KEY, function_module_);
  native_method(Class_Function, PARENT_CLASS, function_parent_class_);
  native_method(Class_Function, global_intern("is_method"),
                function_is_method_);
  native_method(Class_FunctionRef, MODULE_KEY, function_ref_module_);
  native_method(Class_Function, NAME_KEY, function_name_);
  native_method(Class_FunctionRef, NAME_KEY, function_ref_name_);
  native_method(Class_FunctionRef, OBJ_KEY, function_ref_obj_);
  native_method(Class_FunctionRef, global_intern("func"), function_ref_func_);
}

void builtin_add_range_(Module *builtin) {
  Class_Range =
      native_class(builtin, RANGE_CLASS_NAME, range_init_, range_delete_);
  native_method(Class_Range, CONSTRUCTOR_KEY, range_constructor_);
  native_method(Class_Range, global_intern("start"), range_start_);
  native_method(Class_Range, global_intern("inc"), range_inc_);
  native_method(Class_Range, global_intern("end"), range_end_);
  Class_Range->_copy_fn = (ObjCopyFn)range_copy_;
}

void builtin_add_isolate_(Module *builtin) {
  Class_Isolate =
      native_class(builtin, ISOLATE_NAME, isolate_init_, isolate_delete_);
  native_method(Class_Isolate, global_intern("start"), isolate_start_);
  Class_Frame = native_class(builtin, FRAME_NAME, frame_init_, frame_delete_);
  Class_Scope = native_class(builtin, SCOPE_NAME, scope_init_, scope_delete_);
  Class_ActorRef =
      native_class(builtin, ACTOR_REF_CLASS_NAME, actor_init_, actor_delete_);
  Class_ActorRef->_copy_fn = (ObjCopyFn)actor_copy_;
  native_method(Class_ActorRef, global_intern("isolate"), actor_isolate_);
}

void builtin_add_native(ModuleManager *mm, Module *builtin) {
  builtin_classes(mm->_heap, builtin);

  builtin_add_isolate_(builtin);

  native_function(builtin, global_intern("__collect_garbage"),
                  collect_garbage_);
  native_function(builtin, global_intern("Int"), Int_);
  native_function(builtin, global_intern("Float"), Float_);
  native_function(builtin, global_intern("Bool"), _Bool_);
  native_function(builtin, global_intern("__stringify"), stringify_);
  native_function(builtin, global_intern("__tuple"), tuple_);
  native_function(builtin, global_intern("color"), color_);

  builtin_add_string_(builtin);
  builtin_add_istring_(builtin);
  builtin_add_function_(builtin);
  builtin_add_range_(builtin);

  native_method(Class_Class, MODULE_KEY, class_module_);
  native_method(Class_Class, NAME_KEY, class_name_);
  native_method(Class_Class, SUPER_KEY, class_super_);
  native_method(Class_Class, global_intern("is_subclass_of"),
                class_is_subclass_of_);
  native_method(Class_Class, global_intern("methods"), class_methods_);

  native_method(Class_Class, global_intern("$__set_super"), class_set_super_);
  native_method(Class_Class, global_intern("$set_method"), class_set_method_);
  native_method(Class_Class, FIELDS_KEY, class_fields_);

  native_method(Class_Object, CLASS_KEY, object_class_);
  native_method(Class_Object, SUPER_KEY, object_super_);
  native_method(Class_Object, HASH_KEY, object_hash_);
  native_method(Class_Object, global_intern("copy"), object_copy_);
  native_method(Class_Object, global_intern("$set"), set_member_);
  native_method(Class_Object, global_intern("$get"), get_member_);
  native_method(Class_Object, ADDRESS_INT_KEY, object_address_int_);
  native_method(Class_Object, ADDRESS_HEX_KEY, object_address_hex_);

  native_method(Class_Array, global_intern("len"), array_len_);
  native_method(Class_Array, global_intern("append"), array_append_);
  native_method(Class_Array, global_intern("__remove"), array_remove_);

  native_method(Class_Tuple, global_intern("len"), tuple_len_);

  native_method(Class_Module, NAME_KEY, module_name_);
  native_method(Class_Module, global_intern("functions"), module_functions_);
  native_method(Class_Module, global_intern("classes"), module_classes_);
  native_method(Class_Module, global_intern("has_dl"), module_has_dl_);
  native_method(Class_Module, global_intern("dlcall"), module_dlcall_);
}
