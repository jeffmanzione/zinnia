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
#include "entity/tuple/tuple.h"
#include "vm/intern.h"
#include "vm/process/processes.h"

Object *_string_new(Heap *heap, const char src[], size_t len) {
  Object *str = heap_new(heap, Class_String);
  __string_init(str, src, len);
  return str;
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
      _string_new(task->parent_process->heap, buffer, num_written));
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

Entity _string_len(Task *task, Context *ctx, Object *obj, Entity *args) {
  String *str = (String *)obj->_internal_obj;
  return entity_int(String_size(str));
}

Entity _array_len(Task *task, Context *ctx, Object *obj, Entity *args) {
  Array *arr = (Array *)obj->_internal_obj;
  return entity_int(Array_size(arr));
}

Entity _tuple_len(Task *task, Context *ctx, Object *obj, Entity *args) {
  Tuple *t = (Tuple *)obj->_internal_obj;
  return entity_int(tuple_size(t));
}

Entity _object_class(Task *task, Context *ctx, Object *obj, Entity *args) {
  return entity_object(obj->_class->_reflection);
}

Entity _class_module(Task *task, Context *ctx, Object *obj, Entity *args) {
  return entity_object(obj->_class_obj->_module->_reflection);
}

Entity _function_name(Task *task, Context *ctx, Object *obj, Entity *args) {
  return entity_object(_string_new(task->parent_process->heap,
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
  return entity_object(_string_new(task->parent_process->heap,
                                   obj->_class_obj->_name,
                                   strlen(obj->_class_obj->_name)));
}

Entity _module_name(Task *task, Context *ctx, Object *obj, Entity *args) {
  return entity_object(_string_new(task->parent_process->heap,
                                   obj->_module_obj->_name,
                                   strlen(obj->_module_obj->_name)));
}

Entity _function_ref_name(Task *task, Context *ctx, Object *obj, Entity *args) {
  const char *name = function_ref_get_func(obj)->_name;
  return entity_object(
      _string_new(task->parent_process->heap, name, strlen(name)));
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

void builtin_add_native(Module *builtin) {
  native_function(builtin, intern("__stringify"), _stringify);
  native_method(Class_String, intern("extend"), _string_extend);
  native_method(Class_String, intern("len"), _string_len);
  native_method(Class_Array, intern("len"), _array_len);
  native_method(Class_Tuple, intern("len"), _tuple_len);
  native_method(Class_Object, CLASS_KEY, _object_class);
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