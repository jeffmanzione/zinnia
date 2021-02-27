// module_manager.c
//
// Created on: Jul 10, 2020
//     Author: Jeff Manzione

#include "vm/module_manager.h"

#include "alloc/arena/intern.h"
#include "entity/class/class.h"
#include "entity/class/classes.h"
#include "entity/function/function.h"
#include "entity/module/module.h"
#include "entity/object.h"
#include "entity/string/string_helper.h"
#include "lang/parser/parser.h"
#include "lang/semantics/expression_tree.h"
#include "program/optimization/optimize.h"
#include "program/tape_binary.h"
#include "struct/struct_defaults.h"
#include "util/file/file_info.h"
#include "util/string.h"
#include "vm/intern.h"

typedef struct {
  Module module;
  FileInfo *fi;
} ModuleInfo;

void add_reflection_to_module(ModuleManager *mm, Module *module);

void modulemanager_init(ModuleManager *mm, Heap *heap) {
  ASSERT(NOT_NULL(mm));
  mm->_heap = heap;
  keyedlist_init(&mm->_modules, ModuleInfo, 25);
  set_init_default(&mm->_files_processed);
}

void modulemanager_finalize(ModuleManager *mm) {
  ASSERT(NOT_NULL(mm));
  KL_iter iter = keyedlist_iter(&mm->_modules);
  for (; kl_has(&iter); kl_inc(&iter)) {
    ModuleInfo *module_info = (ModuleInfo *)kl_value(&iter);
    module_finalize(&module_info->module);
    if (NULL != module_info->fi) {
      file_info_delete(module_info->fi);
    }
  }
  keyedlist_finalize(&mm->_modules);
  set_finalize(&mm->_files_processed);
}

bool _hydrate_class(Module *module, ClassRef *cref) {
  ASSERT(NOT_NULL(module), NOT_NULL(cref));
  const Class *super = NULL;
  if (alist_len(&cref->supers) > 0) {
    super =
        module_lookup_class(module, *((char **)alist_get(&cref->supers, 0)));
    // Need to reprocess this later since its super is not yet processed.
    if (NULL == super) {
      return false;
    }
  } else {
    super = Class_Object;
  }
  Class *class = module_add_class(module, cref->name, super);
  KL_iter fields = keyedlist_iter(&cref->field_refs);
  for (; kl_has(&fields); kl_inc(&fields)) {
    FieldRef *fref = (FieldRef *)kl_value(&fields);
    class_add_field(class, fref->name);
  }
  KL_iter funcs = keyedlist_iter(&cref->func_refs);
  for (; kl_has(&funcs); kl_inc(&funcs)) {
    FunctionRef *fref = (FunctionRef *)kl_value(&funcs);
    class_add_function(class, fref->name, fref->index, fref->is_const,
                       fref->is_async);
  }
  return true;
}

ModuleInfo *_modulemanager_hydrate(ModuleManager *mm, Tape *tape) {
  ASSERT(NOT_NULL(mm), NOT_NULL(tape));
  ModuleInfo *module_info;
  ModuleInfo *old = (ModuleInfo *)keyedlist_insert(
      &mm->_modules, tape_module_name(tape), (void **)&module_info);
  if (old != NULL) {
    ERROR("Module by name '%s' already exists.", module_name(&old->module));
  }
  Module *module = &module_info->module;
  module_init(module, tape_module_name(tape), tape);

  KL_iter funcs = tape_functions(tape);
  for (; kl_has(&funcs); kl_inc(&funcs)) {
    FunctionRef *fref = (FunctionRef *)kl_value(&funcs);
    module_add_function(module, fref->name, fref->index, fref->is_const,
                        fref->is_async);
  }

  KL_iter classes = tape_classes(tape);
  Q classes_to_process;
  Q_init(&classes_to_process);
  Map waiting_for_class;
  map_init_default(&waiting_for_class);
  for (; kl_has(&classes); kl_inc(&classes)) {
    ClassRef *cref = (ClassRef *)kl_value(&classes);
    if (!_hydrate_class(module, cref)) {
      *Q_add_last(&classes_to_process) = cref;
    }
  }
  while (Q_size(&classes_to_process) > 0) {
    ClassRef *cref = (ClassRef *)Q_pop(&classes_to_process);
    if (!_hydrate_class(module, cref)) {
      *Q_add_last(&classes_to_process) = cref;
    }
  }
  Q_finalize(&classes_to_process);
  map_finalize(&waiting_for_class);
  return module_info;
}

void add_reflection_to_function(Heap *heap, Object *parent, Function *func) {
  if (NULL == func->_reflection) {
    func->_reflection = heap_new(heap, Class_Function);
  }
  func->_reflection->_function_obj = func;
  object_set_member_obj(heap, parent, func->_name, func->_reflection);
}

void _add_reflection_to_class(Heap *heap, Module *module, Class *class) {
  ASSERT(NOT_NULL(heap), NOT_NULL(module), NOT_NULL(class));
  if (NULL == class->_reflection) {
    class->_reflection = heap_new(heap, Class_Class);
  }
  if (NULL == class->_super && class != Class_Object) {
    class->_super = Class_Object;
  }
  class->_reflection->_class_obj = class;
  object_set_member_obj(heap, module->_reflection, class->_name,
                        class->_reflection);

  KL_iter funcs = class_functions(class);
  for (; kl_has(&funcs); kl_inc(&funcs)) {
    Function *func = (Function *)kl_value(&funcs);
    add_reflection_to_function(heap, class->_reflection, func);
  }

  Object *field_arr = heap_new(heap, Class_Array);
  object_set_member_obj(heap, class->_reflection, FIELDS_PRIVATE_KEY,
                        field_arr);
  KL_iter fields = class_fields(class);
  for (; kl_has(&fields); kl_inc(&fields)) {
    Field *field = (Field *)kl_value(&fields);
    Entity str =
        entity_object(string_new(heap, field->name, strlen(field->name)));
    array_add(heap, field_arr, &str);
  }
}

void add_reflection_to_module(ModuleManager *mm, Module *module) {
  ASSERT(NOT_NULL(mm), NOT_NULL(module));
  module->_reflection = heap_new(mm->_heap, Class_Module);
  module->_reflection->_module_obj = module;
  KL_iter funcs = module_functions(module);
  for (; kl_has(&funcs); kl_inc(&funcs)) {
    Function *func = (Function *)kl_value(&funcs);
    add_reflection_to_function(mm->_heap, func->_module->_reflection, func);
  }
  KL_iter classes = module_classes(module);
  for (; kl_has(&classes); kl_inc(&classes)) {
    Class *class = (Class *)kl_value(&classes);
    _add_reflection_to_class(mm->_heap, module, class);
  }
}

Module *_read_jl(ModuleManager *mm, const char fn[]) {
  FileInfo *fi = file_info(fn);
  SyntaxTree stree = parse_file(fi);
  ExpressionTree *etree = populate_expression(&stree);

  Tape *tape = tape_create();
  produce_instructions(etree, tape);
  delete_expression(etree);
  syntax_tree_delete(&stree);

  tape = optimize(tape);
  ModuleInfo *module_info = _modulemanager_hydrate(mm, tape);
  module_info->fi = fi;
  return &module_info->module;
}

Module *_read_ja(ModuleManager *mm, const char fn[]) {
  FileInfo *fi = file_info(fn);
  Lexer lexer;
  lexer_init(&lexer, fi, true);
  Q *tokens = lex(&lexer);

  Tape *tape = tape_create();
  tape_read(tape, tokens);
  ModuleInfo *module_info = _modulemanager_hydrate(mm, tape);
  module_info->fi = fi;

  lexer_finalize(&lexer);
  return &module_info->module;
}

Module *_read_jb(ModuleManager *mm, const char fn[]) {
  FILE *file = fopen(fn, "rb");
  if (NULL == file) {
    ERROR("Cannot open file '%s'. Exiting...", fn);
  }
  Tape *tape = tape_create();
  tape_read_binary(tape, file);
  ModuleInfo *module_info = _modulemanager_hydrate(mm, tape);
  return &module_info->module;
}

Module *mm_read_helper(ModuleManager *mm, const char fn[]) {
  char *interned_fn = intern(fn);
  if (set_lookup(&mm->_files_processed, interned_fn)) {
    return NULL;
  }
  set_insert(&mm->_files_processed, interned_fn);
  if (ends_with(interned_fn, ".jb")) {
    return _read_jb(mm, interned_fn);
  } else if (ends_with(fn, ".ja")) {
    return _read_ja(mm, interned_fn);
  } else if (ends_with(fn, ".jv")) {
    return _read_jl(mm, interned_fn);
  } else {
    ERROR("Unknown file type.");
  }
  return NULL;
}

Module *modulemanager_read(ModuleManager *mm, const char fn[]) {
  Module *module = mm_read_helper(mm, fn);
  if (NULL == module) {
    return NULL;
  }
  add_reflection_to_module(mm, module);
  return module;
}

Module *modulemanager_lookup(ModuleManager *mm, const char fn[]) {
  ModuleInfo *mi = (ModuleInfo *)keyedlist_lookup(&mm->_modules, fn);
  if (NULL == mi) {
    return NULL;
  }
  return &mi->module;
}

const FileInfo *modulemanager_get_fileinfo(const ModuleManager *mm,
                                           const Module *m) {
  ASSERT(NOT_NULL(mm), NOT_NULL(m), NOT_NULL(m->_name));
  const ModuleInfo *mi =
      (ModuleInfo *)keyedlist_lookup((KeyedList *)&mm->_modules, m->_name);
  if (NULL == mi) {
    return NULL;
  }
  return mi->fi;
}

void modulemanager_update_module(ModuleManager *mm, Module *m,
                                 Map *new_classes) {
  ASSERT(NOT_NULL(mm), NOT_NULL(m), NOT_NULL(new_classes));
  Tape *tape = (Tape *)m->_tape;  // bless

  KL_iter classes = tape_classes(tape);
  Q classes_to_process;
  Q_init(&classes_to_process);

  for (; kl_has(&classes); kl_inc(&classes)) {
    ClassRef *cref = (ClassRef *)kl_value(&classes);
    Class *c = (Class *)module_lookup_class(m, cref->name);  // bless
    if (NULL != c) {
      continue;
    }
    if (_hydrate_class(m, cref)) {
      c = (Class *)module_lookup_class(m, cref->name);  // bless
      map_insert(new_classes, cref->name, c);
      _add_reflection_to_class(mm->_heap, m, c);
    } else {
      *Q_add_last(&classes_to_process) = cref;
    }
  }
  while (Q_size(&classes_to_process) > 0) {
    ClassRef *cref = (ClassRef *)Q_pop(&classes_to_process);
    Class *c = (Class *)module_lookup_class(m, cref->name);  // bless
    if (_hydrate_class(m, cref)) {
      c = (Class *)module_lookup_class(m, cref->name);  // bless
      map_insert(new_classes, cref->name, c);
      _add_reflection_to_class(mm->_heap, m, c);
    } else {
      *Q_add_last(&classes_to_process) = cref;
    }
  }
}