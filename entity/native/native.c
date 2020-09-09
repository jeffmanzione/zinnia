// native.c
//
// Created on: Aug 23, 2020
//     Author: Jeff Manzione

#include "entity/native/native.h"

#include <stdbool.h>

#include "entity/class/classes.h"

Function *native_method(Class *class, const char name[], NativeFn native_fn) {
  Function *fn = class_add_function(class, name, 0);
  fn->_is_native = true;
  fn->_native_fn = native_fn;
  return fn;
}

Function *native_function(Module *module, const char name[],
                          NativeFn native_fn) {
  Function *fn = module_add_function(module, name, 0);
  fn->_is_native = true;
  fn->_native_fn = native_fn;
  return fn;
}

Class *native_class(Module *module, const char name[], ObjInitFn init_fn,
                    ObjDelFn del_fn) {
  Class *cls = module_add_class(module, name, Class_Object);
  cls->_init_fn = init_fn;
  cls->_delete_fn = del_fn;
  return cls;
}