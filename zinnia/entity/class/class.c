// class.c
//
// Created on: May 31, 2020
//     Author: Jeff Manzione

#include "zinnia/entity/class/class.h"

#include "c-data-structures/stable_maplike.h"
#include "zinnia/alloc/alloc.h"
#include "zinnia/entity/function/function.h"
#include "zinnia/entity/object.h"
#include "zinnia/util/error.h"


Class *class_init(Class *cls, const char name[], const Class *super,
                  const Module *module) {
  ASSERT(cls != NULL);
  ASSERT(name != NULL);
  ASSERT(module != NULL);
  cls->_name = name;
  cls->_super = super;
  cls->_module = module;
  cls->_reflection = NULL;
  cls->_init_fn = NULL;
  cls->_delete_fn = NULL;
  cls->_print_fn = NULL;
  cls->_copy_fn = NULL;
  FunctionMap_init(&cls->_functions, hash_interned_string,
                   compare_interned_strings);
  FieldMap_init(&cls->_fields, hash_interned_string, compare_interned_strings);
  return cls;
}

void class_finalize(Class *cls) {
  ASSERT(cls != NULL);
  FunctionMap_finalize(&cls->_functions);
  FieldMap_finalize(&cls->_fields);
}

Function *class_add_function(Class *cls, const char name[], uint32_t ins_pos,
                             bool is_const, bool is_async) {
  ASSERT(cls != NULL);
  ASSERT(name != NULL);
  Function *f;
  if (!FunctionMap_insert(&cls->_functions, name, sizeof(char *), &f)) {
    FATALF(
        "Adding function %s to class %s that already has a function by this "
        "name.",
        name, cls->_name);
  }
  function_init(f, name, cls->_module, ins_pos, is_anon(name), is_const,
                is_async);
  f->_parent_class = cls;
  return f;
}

Field *class_add_field(Class *cls, const char name[]) {
  ASSERT(cls != NULL);
  ASSERT(name != NULL);
  Field *f;
  if (!FieldMap_insert(&cls->_fields, name, sizeof(char *), &f)) {
    FATALF(
        "Adding function %s to class %s that already has a function by this "
        "name.",
        name, cls->_name);
  }
  f->name = name;
  return f;
}

FunctionMapIterator class_functions(Class *cls) {
  FunctionMapIterator it;
  FunctionMap_iterator(&it, &cls->_functions);
  return it;
}

FieldMapIterator class_fields(Class *cls) {
  FieldMapIterator it;
  FieldMap_iterator(&it, &cls->_fields);
  return it;
}

const Function *class_get_function(const Class *cls, const char name[]) {
  const Class *class = cls;
  while (NULL != class) {
    const Function *f =
        FunctionMap_find_ref(&class->_functions, name, sizeof(char *));
    if (NULL != f) {
      return f;
    }
    class = class->_super;
  }
  return NULL;
}

bool inherits_from(const Class *class, Class *possible_super) {
  for (;;) {
    if (NULL == class) {
      return false;
    }
    if (class == possible_super) {
      return true;
    }
    class = class->_super;
  }
}