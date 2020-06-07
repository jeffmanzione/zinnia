// class.c
//
// Created on: May 31, 2020
//     Author: Jeff Manzione

#include "entity/class/class.h"

#include "alloc/alloc.h"
#include "debug/debug.h"
#include "entity/function/function.h"
#include "entity/module/module.h"
#include "entity/object.h"
#include "struct/keyed_list.h"

Class *class_create(const char name[], const Class *super,
                    const Module *module) {
  ASSERT(NOT_NULL(name), NOT_NULL(super), NOT_NULL(module));
  Class *cls = ALLOC2(Class);
  cls->_name = name;
  cls->_super = super;
  cls->_module = module;
  keyedlist_init(&cls->_functions, Function, DEFAULT_ARRAY_SZ);
  return cls;
}

void class_delete(Class *cls) {
  ASSERT(NOT_NULL(cls));
  keyedlist_finalize(&cls->_functions);
  DEALLOC(cls);
}

Function *class_add_function(Class *cls, const char name[]) {
  ASSERT(NOT_NULL(cls), NOT_NULL(name));
  Function *f;
  Function *old = keyedlist_insert(&cls->_functions, name, &f);
  if (NULL != old) {
    ERROR(
        "Adding function %s to class %s that already has a function by this "
        "name.",
        name, cls->_name);
  }
  function_init(f, name, cls->_module);
  return f;
}
