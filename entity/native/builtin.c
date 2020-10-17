// builtin.c
//
// Created on: Aug 29, 2020
//     Author: Jeff Manzione

#include "entity/native/builtin.h"

#include "alloc/arena/intern.h"
#include "entity/array/array.h"
#include "entity/class/classes.h"
#include "entity/entity.h"
#include "entity/native/native.h"
#include "entity/object.h"
#include "entity/primitive.h"
#include "entity/string/string.h"
#include "entity/string/string_helper.h"
#include "entity/tuple/tuple.h"
#include "lang/lexer/file_info.h"
#include "vm/intern.h"
#include "vm/process/processes.h"
#include "util/util.h"

#define min(x, y) ((x) > (y) ? (y) : (x))

static Class *Class_Range;

typedef struct {
  int32_t start;
  int32_t end;
  int32_t inc;
} _Range;

Entity _Int(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (NULL == args) {
    return entity_int(0);
  }
  switch (args->type) {
    case NONE:
      return entity_int(0);
    case OBJECT:
      // Is this the right way to handle this?
      return entity_int(0);
    case PRIMITIVE:
      switch (ptype(&args->pri)) {
        case CHAR:
          return entity_int(pchar(&args->pri));
        case INT:
          return *args;
        case FLOAT:
          return entity_int(pfloat(&args->pri));
        default:
          ERROR("Unknown primitive type.");
      }
    default:
      ERROR("Unknown type.");
  }
  return entity_int(0);
}

Entity _stringify(Task *task, Context *ctx, Object *obj, Entity *args) {
  ASSERT(NOT_NULL(args), PRIMITIVE == args->type);
  Primitive val = args->pri;
  static const int BUFFER_SIZE = 256;
  char buffer[BUFFER_SIZE];
  int num_written = 0;
  switch (ptype(&val)) {
    case INT:
      num_written = snprintf(buffer, BUFFER_SIZE, "%d", pint(&val));
      break;
    case FLOAT:
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
    ERROR("Cannot extend a string with something not a string.");
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
  if (PRIMITIVE != args->type || INT != ptype(&args->pri)) {
    ERROR("Bad string index input");
  }
  String *self = (String *)obj->_internal_obj;
  int32_t index = pint(&args->pri);
  if (index < 0 || index >= String_size(self)) {
    ERROR("Index out of bounds.");
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

Entity _array_len(Task *task, Context *ctx, Object *obj, Entity *args) {
  Array *arr = (Array *)obj->_internal_obj;
  return entity_int(Array_size(arr));
}

Entity _array_append(Task *task, Context *ctx, Object *obj, Entity *args) {
  array_add(task->parent_process->heap, obj, args);
  return entity_object(obj);
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
    ERROR("Input to range() is not a tuple.");
  }
  Tuple *t = (Tuple *)args->obj->_internal_obj;
  if (3 != tuple_size(t)) {
    ERROR("Invalid tuple size for range(). Was %d", tuple_size(t));
  }
  const Entity *first = tuple_get(t, 0);
  const Entity *second = tuple_get(t, 1);
  const Entity *third = tuple_get(t, 2);
  if (PRIMITIVE != first->type || INT != ptype(&first->pri)) {
    ERROR("Input to range() is invalid.");
  }
  if (PRIMITIVE != first->type || INT != ptype(&second->pri)) {
    ERROR("Input to range() is invalid.");
  }
  if (PRIMITIVE != first->type || INT != ptype(&third->pri)) {
    ERROR("Input to range() is invalid.");
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

void builtin_add_native(Module *builtin) {
  Class_Range = native_class(builtin, RANGE_CLASS_NAME, _range_init, _range_delete);
  native_method(Class_Range, CONSTRUCTOR_KEY, _range_constructor);
  native_method(Class_Range, intern("start"), _range_start);
  native_method(Class_Range, intern("inc"), _range_inc);
  native_method(Class_Range, intern("end"), _range_end);

  native_function(builtin, intern("Int"), _Int);
  native_function(builtin, intern("__stringify"), _stringify);
  native_method(Class_String, intern("extend"), _string_extend);
  native_method(Class_String, CMP_FN_NAME, _string_cmp);
  native_method(Class_String, EQ_FN_NAME, _string_eq);
  native_method(Class_String, NEQ_FN_NAME, _string_neq);
  native_method(Class_String, ARRAYLIKE_INDEX_KEY, _string_index);
  native_method(Class_String, intern("len"), _string_len);
  native_method(Class_String, HASH_KEY, _string_hash);
  native_method(Class_Array, intern("len"), _array_len);
  native_method(Class_Array, intern("append"), _array_append);
  native_method(Class_Tuple, intern("len"), _tuple_len);
  native_method(Class_Object, CLASS_KEY, _object_class);
  native_method(Class_Object, HASH_KEY, _object_hash);
  native_method(Class_Function, MODULE_KEY, _function_module);
  native_method(Class_Function, PARENT_CLASS, _function_parent_class);
  native_method(Class_Function, intern("is_method"), _function_is_method);
  native_method(Class_FunctionRef, MODULE_KEY, _function_ref_module);
  native_method(Class_Class, MODULE_KEY, _class_module);
  native_method(Class_Function, NAME_KEY, _function_name);
  native_method(Class_FunctionRef, NAME_KEY, _function_ref_name);
  native_method(Class_FunctionRef, OBJ_KEY, _function_ref_obj);
  native_method(Class_FunctionRef, intern("func"), _function_ref_func);
  native_method(Class_Class, NAME_KEY, _class_name);
  native_method(Class_Module, NAME_KEY, _module_name);
}