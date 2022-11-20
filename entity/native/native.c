// native.c
//
// Created on: Aug 23, 2020
//     Author: Jeff Manzione

#include "entity/native/native.h"

#include <stdbool.h>

#include "entity/class/classes_def.h"
#include "entity/string/string.h"

Function *native_method(Class *class, const char name[], NativeFn native_fn) {
  Function *fn = class_add_function(class, name, 0, false, false);
  fn->_is_native = true;
  fn->_native_fn = native_fn;
  return fn;
}

Function *native_background_method(Class *class, const char *name,
                                   NativeFn native_fn) {
  Function *fn = native_method(class, name, native_fn);
  fn->_is_background = true;
  fn->_is_async = true;
  return fn;
}

Function *native_function(Module *module, const char name[],
                          NativeFn native_fn) {

  Function *fn = module_add_function(module, name, 0, false, false);
  fn->_is_native = true;
  fn->_native_fn = native_fn;
  return fn;
}

Function *native_background_function(Module *module, const char name[],
                                     NativeFn native_fn) {
  Function *fn = native_function(module, name, native_fn);
  fn->_is_background = true;
  fn->_is_async = true;
  return fn;
}

Class *native_class(Module *module, const char name[], ObjInitFn init_fn,
                    ObjDelFn del_fn) {
  Class *cls = module_add_class(module, name, Class_Object);
  cls->_init_fn = init_fn;
  cls->_delete_fn = del_fn;
  return cls;
}

Object *native_background_new(Process *process, Class *class) {
  Object *object;
  SYNCHRONIZED(process->heap_access_lock,
               { object = heap_new(process->heap, class); });
  return object;
}

Object *native_background_string_new(Process *process, const char src[],
                                     size_t len) {
  Object *str = native_background_new(process, Class_String);
  __string_init(str, src, len);
  return str;
}
