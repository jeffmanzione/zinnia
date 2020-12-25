// class.c
//
// Created on: May 31, 2020
//     Author: Jeff Manzione

#include "entity/class/class.h"

#include "alloc/alloc.h"
#include "debug/debug.h"
#include "entity/function/function.h"
#include "entity/object.h"
#include "struct/keyed_list.h"

Class *class_init(Class *cls, const char name[], const Class *super,
                  const Module *module) {
  ASSERT(NOT_NULL(cls), NOT_NULL(name), NOT_NULL(module));
  cls->_name = name;
  cls->_super = super;
  cls->_module = module;
  cls->_reflection = NULL;
  cls->_init_fn = NULL;
  cls->_delete_fn = NULL;
  cls->_print_fn = NULL;
  cls->_copy_fn = NULL;
  keyedlist_init(&cls->_functions, Function, 16);
  return cls;
}

void class_finalize(Class *cls) {
  ASSERT(NOT_NULL(cls));
  keyedlist_finalize(&cls->_functions);
}

Function *class_add_function(Class *cls, const char name[], uint32_t ins_pos,
                             bool is_const, bool is_async) {
  ASSERT(NOT_NULL(cls), NOT_NULL(name));
  Function *f;
  Function *old =
      (Function *)keyedlist_insert(&cls->_functions, name, (void **)&f);
  if (NULL != old) {
    ERROR("Adding function %s to class %s that already has a function by this "
          "name.",
          name, cls->_name);
  }
  function_init(f, name, cls->_module, ins_pos, is_anon(name), is_const,
                is_async);
  f->_parent_class = cls;
  return f;
}

inline KL_iter class_functions(Class *cls) {
  return keyedlist_iter(&cls->_functions);
}

const Function *class_get_function(const Class *cls, const char name[]) {
  const Class *class = cls;
  while (NULL != class) {
    const Function *f =
        (Function *)keyedlist_lookup((KeyedList *)&class->_functions, name);
    if (NULL != f) {
      return f;
    }
    class = class->_super;
  }
  return NULL;
}