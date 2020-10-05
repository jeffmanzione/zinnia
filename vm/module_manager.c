// module_manager.h
//
// Created on: Jul 10, 2020
//     Author: Jeff Manzione

#include "vm/module_manager.h"

#include "entity/class/class.h"
#include "entity/class/classes.h"
#include "entity/function/function.h"
#include "entity/module/module.h"
#include "entity/module/modules.h"
#include "entity/native/builtin.h"
#include "entity/native/error.h"
#include "entity/native/io.h"
#include "entity/object.h"
#include "lang/lexer/file_info.h"
#include "lang/parser/parser.h"
#include "lang/semantics/expression_tree.h"
#include "vm/intern.h"


typedef struct {
  Module module;
  FileInfo *fi;
} ModuleInfo;

void _read_builtin(ModuleManager *mm, Heap *heap);
Module *_read_helper(ModuleManager *mm, const char fn[]);
void _add_reflection_to_module(ModuleManager *mm, Module *module);

void modulemanager_init(ModuleManager *mm, Heap *heap) {
  ASSERT(NOT_NULL(mm));
  mm->_heap = heap;
  keyedlist_init(&mm->_modules, ModuleInfo, DEFAULT_ARRAY_SZ);
  _read_builtin(mm, heap);
}

void modulemanager_finalize(ModuleManager *mm) {
  ASSERT(NOT_NULL(mm));
  KL_iter iter = keyedlist_iter(&mm->_modules);
  for (; kl_has(&iter); kl_inc(&iter)) {
    ModuleInfo *module_info = (ModuleInfo *)kl_value(&iter);
    module_finalize(&module_info->module);
    file_info_delete(module_info->fi);
  }
  keyedlist_finalize(&mm->_modules);
}

void _read_builtin(ModuleManager *mm, Heap *heap) {
  // builtin.jl
  Module_builtin = _read_helper(mm, "lib/builtin.jl");
  builtin_classes(heap, Module_builtin);
  builtin_add_native(Module_builtin);
  _add_reflection_to_module(mm, Module_builtin);
  // io.jl
  Module_io = _read_helper(mm, "lib/io.jl");
  io_add_native(Module_io);
  _add_reflection_to_module(mm, Module_io);
  // error.jl
  Module_error = _read_helper(mm, "lib/error.jl");
  error_add_native(Module_error);
  _add_reflection_to_module(mm, Module_error);
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
    module_add_function(module, fref->name, fref->index);
  }

  KL_iter classes = tape_classes(tape);
  for (; kl_has(&classes); kl_inc(&classes)) {
    ClassRef *cref = (ClassRef *)kl_value(&classes);
    // TODO: Handle subclasses.
    Class *class = module_add_class(module, cref->name, Class_Object);

    KL_iter funcs = keyedlist_iter(&cref->func_refs);
    for (; kl_has(&funcs); kl_inc(&funcs)) {
      FunctionRef *fref = (FunctionRef *)kl_value(&funcs);
      class_add_function(class, fref->name, fref->index);
    }
  }
  return module_info;
}

void _add_reflection_to_function(Heap *heap, Object *parent, Function *func) {
  if (NULL == func->_reflection) {
    func->_reflection = heap_new(heap, Class_Function);
  }
  func->_reflection->_function_obj = func;
  object_set_member_obj(heap, parent, func->_name, func->_reflection);
}

void _add_reflection_to_class(Heap *heap, Module *module, Class *class) {
  if (NULL == class->_reflection) {
    class->_reflection = heap_new(heap, Class_Class);
  }
  class->_reflection->_class_obj = class;
  object_set_member_obj(heap, module->_reflection, class->_name,
                        class->_reflection);

  KL_iter funcs = class_functions(class);
  for (; kl_has(&funcs); kl_inc(&funcs)) {
    Function *func = (Function *)kl_value(&funcs);
    _add_reflection_to_function(heap, class->_reflection, func);
  }
}

void _add_reflection_to_module(ModuleManager *mm, Module *module) {
  ASSERT(NOT_NULL(mm), NOT_NULL(module));
  module->_reflection = heap_new(mm->_heap, Class_Module);
  module->_reflection->_module_obj = module;
  KL_iter funcs = module_functions(module);
  for (; kl_has(&funcs); kl_inc(&funcs)) {
    Function *func = (Function *)kl_value(&funcs);
    _add_reflection_to_function(mm->_heap, func->_module->_reflection, func);
  }
  KL_iter classes = module_classes(module);
  for (; kl_has(&classes); kl_inc(&classes)) {
    Class *class = (Class *)kl_value(&classes);
    _add_reflection_to_class(mm->_heap, module, class);
  }
}

Module *_read_helper(ModuleManager *mm, const char fn[]) {
  FileInfo *fi = file_info(fn);
  SyntaxTree stree = parse_file(fi);
  ExpressionTree *etree = populate_expression(&stree);
  Tape *tape = tape_create();
  produce_instructions(etree, tape);
  ModuleInfo *module_info = _modulemanager_hydrate(mm, tape);
  module_info->fi = fi;
  delete_expression(etree);
  syntax_tree_delete(&stree);
  return &module_info->module;
}

Module *modulemanager_read(ModuleManager *mm, const char fn[]) {
  Module *module = _read_helper(mm, fn);
  _add_reflection_to_module(mm, module);
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