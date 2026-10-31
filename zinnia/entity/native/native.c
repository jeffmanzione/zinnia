// native.c
//
// Created on: Aug 23, 2020
//     Author: Jeff Manzione

#include "zinnia/entity/native/native.h"

#include <stdbool.h>

#include "zinnia/entity/class/classes_def.h"
#include "zinnia/entity/string/string.h"
#include "zinnia/util/platform.h"

#ifdef OS_WINDOWS
static __declspec(thread) Frame *blocking_frame_ = NULL;
#else
static __thread Frame *blocking_frame_ = NULL;
#endif

void native_set_blocking_frame(Frame *frame) { blocking_frame_ = frame; }

void native_pin_blocking_object(Isolate *isolate, Object *obj) {
  if (NULL != blocking_frame_ && blocking_frame_->parent_isolate == isolate) {
    heap_inc_edge(isolate->heap, blocking_frame_->_reflection, obj);
  }
}

Function *native_method(Class *class, const char name[], NativeFn native_fn) {
  Function *fn = class_add_function(class, name, 0, false, false);
  fn->_is_native = true;
  fn->_native_fn = native_fn;
  return fn;
}

Function *native_blocking_method(Class *class, const char *name,
                                 NativeFn native_fn) {
  Function *fn = native_method(class, name, native_fn);
  fn->_is_blocking = true;
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

Function *native_blocking_function(Module *module, const char name[],
                                   NativeFn native_fn) {
  Function *fn = native_function(module, name, native_fn);
  fn->_is_blocking = true;
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

Object *native_blocking_new(Isolate *isolate, Class *class) {
  Object *object;
  SYNCHRONIZED(isolate->heap_access_lock, {
    object = heap_new(isolate->heap, class);
    native_pin_blocking_object(isolate, object);
  });
  return object;
}

Object *native_blocking_string_new(Isolate *isolate, const char src[],
                                   size_t len) {
  Object *str = native_blocking_new(isolate, Class_String);
  string_init__(str, src, len);
  return str;
}
