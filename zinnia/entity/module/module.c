#include "zinnia/entity/module/module.h"

#include "zinnia/alloc/alloc.h"
#include "zinnia/entity/class/class.h"
#include "zinnia/entity/function/function.h"
#include "zinnia/program/tape.h"
#include "zinnia/util/error.h"

void module_init(Module *module, const char name[], const char full_path[],
                 const char relative_path[], const char key[], Tape *tape,
                 DlHandle dl) {
  module->_name = name;
  module->_full_path = full_path;
  module->_relative_path = relative_path;
  module->_key = key;
  module->_tape = tape;
  module->_reflection = NULL;
  module->_is_initialized = false;
  module->dl = dl;

  ClassMap_init(&module->_classes, hash_interned_string,
                compare_interned_strings);
  FunctionMap_init(&module->_functions, hash_interned_string,
                   compare_interned_strings);

  module->_write_mutex = mutex_create();
}

void module_finalize(Module *module) {
  ClassMapIterator class_iter;
  ClassMap_iterator(&class_iter, &module->_classes);
  for (; ClassMap_has_entry(&class_iter); ClassMap_next_entry(&class_iter)) {
    Class *class = ClassMap_mutable_value(&class_iter);
    class_finalize(class);
  }
  ClassMap_finalize(&module->_classes);

  FunctionMapIterator func_iter;
  FunctionMap_iterator(&func_iter, &module->_functions);
  for (; FunctionMap_has_entry(&func_iter);
       FunctionMap_next_entry(&func_iter)) {
    Function *function = FunctionMap_mutable_value(&func_iter);
    function_finalize(function);
  }
  FunctionMap_finalize(&module->_functions);
  if (module->_tape != NULL) {
    tape_delete((Tape *)module->_tape);  // Bless
  }
  mutex_close(module->_write_mutex);
}

const char *module_name(const Module *const module) { return module->_name; }

const Tape *module_tape(const Module *const module) { return module->_tape; }

Function *module_add_function(Module *module, const char name[],
                              uint32_t ins_pos, bool is_const, bool is_async) {
  ASSERT(module != NULL);
  ASSERT(name != NULL);
  Function *f;
  if (!FunctionMap_insert(&module->_functions, name, sizeof(char *), &f)) {
    FATALF(
        "Adding function %s to module %s that already has a function by this "
        "name.",
        name, module->_name);
  }
  function_init(f, name, module, ins_pos, is_anon(name), is_const, is_async);
  return f;
}

Class *module_add_class(Module *module, const char name[], const Class *super) {
  ASSERT(module != NULL);
  ASSERT(name != NULL);
  Class *c;
  if (!ClassMap_insert(&module->_classes, name, sizeof(char *), &c)) {
    return c;
  }
  class_init(c, name, super, module);
  return c;
}

const Class *module_lookup_class(const Module *module, const char name[]) {
  return ClassMap_find_ref(&module->_classes, name, sizeof(char *));
}

Object *module_lookup(Module *module, const char name[]) {
  Class *class = ClassMap_find_ref(&module->_classes, name, sizeof(char *));
  if (NULL != class && NULL != class->_reflection) {
    return class->_reflection;
  }
  Function *func =
      FunctionMap_find_ref(&module->_functions, name, sizeof(char *));
  if (NULL != func && NULL != func->_reflection) {
    return func->_reflection;
  }
  return NULL;
}

FunctionMapIterator module_functions(Module *module) {
  FunctionMapIterator it;
  FunctionMap_iterator(&it, &module->_functions);
  return it;
}

ClassMapIterator module_classes(Module *module) {
  ClassMapIterator it;
  ClassMap_iterator(&it, &module->_classes);
  return it;
}