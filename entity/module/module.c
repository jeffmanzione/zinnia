#include "entity/module/module.h"

#include "alloc/alloc.h"
#include "debug/debug.h"
#include "entity/class/class.h"
#include "entity/function/function.h"
#include "program/tape.h"
#include "struct/struct_defaults.h"

void module_init(Module *module, const char name[], Tape *tape) {
  module->_name = name;
  module->_tape = tape;
  module->_reflection = NULL;
  keyedlist_init(&module->_classes, Class, 16);
  keyedlist_init(&module->_functions, Function, 16);
}

void module_finalize(Module *module) {
  KL_iter class_iter = keyedlist_iter(&module->_classes);
  for (; kl_has(&class_iter); kl_inc(&class_iter)) {
    Class *class = (Class *)kl_value(&class_iter);
    class_finalize(class);
  }
  keyedlist_finalize(&module->_classes);
  KL_iter func_iter = keyedlist_iter(&module->_functions);
  for (; kl_has(&func_iter); kl_inc(&func_iter)) {
    Function *function = (Function *)kl_value(&func_iter);
    function_finalize(function);
  }
  keyedlist_finalize(&module->_functions);
  if (module->_tape != NULL) {
    tape_delete((Tape *)module->_tape);  // Bless
  }
}

inline const char *module_name(const Module *const module) {
  return module->_name;
}

inline const Tape *module_tape(Module *module) { return module->_tape; }

Function *module_add_function(Module *module, const char name[],
                              uint32_t ins_pos) {
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
  function_init(f, name, module, ins_pos);
  return f;
}

Class *module_add_class(Module *module, const char name[], const Class *super) {
  ASSERT(NOT_NULL(module), NOT_NULL(name));
  Class *c;
  Class *old = (Class *)keyedlist_insert(&module->_classes, name, (void **)&c);
  if (NULL != old) {
    return old;
  }
  class_init(c, name, super, module);
  return c;
}

Object *module_lookup(Module *module, const char name[]) {
  Class *class = keyedlist_lookup(&module->_classes, name);
  if (NULL != class && NULL != class->_reflection) {
    return class->_reflection;
  }
  Function *func = keyedlist_lookup(&module->_functions, name);
  if (NULL != func && NULL != func->_reflection) {
    return func->_reflection;
  }
  return NULL;
}

inline KL_iter module_functions(Module *module) {
  return keyedlist_iter(&module->_functions);
}

inline KL_iter module_classes(Module *module) {
  return keyedlist_iter(&module->_classes);
}