#include "entity/module/module.h"

#include "alloc/alloc.h"
#include "debug/debug.h"
#include "entity/function/function.h"
#include "struct/struct_defaults.h"

Module *module_create(const char name[]) {
  Module *m = ALLOC2(Module);
  m->_name = name;
  keyedlist_init(&m->_classes, Class, DEFAULT_ARRAY_SZ);
  keyedlist_init(&m->_functions, Function, DEFAULT_ARRAY_SZ);
  return m;
}

void module_delete(Module *module) {
  keyedlist_finalize(&module->_classes);
  keyedlist_finalize(&module->_functions);
  DEALLOC(module);
}

Function *module_add_function(Module *module, const char name[]) {
  ASSERT(NOT_NULL(module), NOT_NULL(name));
  Function *f;
  Function *old =
      (Function *)keyedlist_insert(&module->_functions, name, (void **)&f);
  if (NULL != old) {
    ERROR(
        "Adding function %s to module %s that already has a function by this "
        "name.",
        name, module->_name);
  }
  function_init(f, name, module);
  return f;
}

Class *module_add_class(Module *module, const char name[]) {
  ASSERT(NOT_NULL(module), NOT_NULL(name));
  Class *c;
  Class *old = (Class *)keyedlist_insert(&module->_classes, name, (void **)&c);
  if (NULL != old) {
    ERROR(
        "Adding clas %s to module %s that already has a function by this "
        "name.",
        name, module->_name);
  }
  return c;
}