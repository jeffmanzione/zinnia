// module_manager.h
//
// Created on: Jul 10, 2020
//     Author: Jeff Manzione

#include "vm/module_manager.h"

#include "entity/class/class.h"
#include "entity/class/classes.h"
#include "entity/function/function.h"
#include "entity/module/module.h"
#include "entity/object.h"
#include "lang/lexer/file_info.h"
#include "lang/parser/parser.h"
#include "lang/semantics/expression_tree.h"
#include "vm/intern.h"

Module *Module_builtin;

typedef struct {
  Module module;
  FileInfo *fi;
} ModuleInfo;

void _read_builtin(ModuleManager *mm, Heap *heap);
Module *_read_helper(ModuleManager *mm, const char fn[]);
void _add_reflection(ModuleManager *mm, Module *module);

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
  Module_builtin = _read_helper(mm, "lib/builtin.jl");
  init_classes(heap, Module_builtin);
  Module_builtin->_reflection = heap_new(mm->_heap, Class_Module);
  _add_reflection(mm, Module_builtin);
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
    Class *class = module_add_class(module, cref->name);
    class_init(class, cref->name, NULL, module);

    KL_iter funcs = keyedlist_iter(&cref->func_refs);
    for (; kl_has(&funcs); kl_inc(&funcs)) {
      FunctionRef *fref = (FunctionRef *)kl_value(&funcs);
      class_add_function(class, fref->name, fref->index);
    }
  }
  return module_info;
}

void _add_reflection(ModuleManager *mm, Module *module) {
  ASSERT(NOT_NULL(mm), NOT_NULL(module));

  KL_iter funcs = module_functions(module);
  for (; kl_has(&funcs); kl_inc(&funcs)) {
    Function *func = (Function *)kl_value(&funcs);
    if (NULL == func->_reflection) {
      func->_reflection = heap_new(mm->_heap, Class_Function);
    }
    func->_reflection->_function_obj = func;
    object_set_member_obj(mm->_heap, module->_reflection, func->_name,
                          func->_reflection);
  }

  KL_iter classes = module_classes(module);
  for (; kl_has(&classes); kl_inc(&classes)) {
    Class *class = (Class *)kl_value(&classes);
    if (NULL == class->_reflection) {
      class->_reflection = heap_new(mm->_heap, Class_Class);
    }
    class->_reflection->_class_obj = class;
    object_set_member_obj(mm->_heap, module->_reflection, class->_name,
                          class->_reflection);

    KL_iter funcs = class_functions(class);
    for (; kl_has(&funcs); kl_inc(&funcs)) {
      Function *func = (Function *)kl_value(&funcs);
      if (NULL == func->_reflection) {
        func->_reflection = heap_new(mm->_heap, Class_Function);
      }
      func->_reflection->_function_obj = func;
      object_set_member_obj(mm->_heap, class->_reflection, func->_name,
                            func->_reflection);
    }
  }
}

Module *_read_helper(ModuleManager *mm, const char fn[]) {
  FileInfo *fi = file_info(fn);
  SyntaxTree stree = parse_file(fi);
  ExpressionTree *etree = populate_expression(&stree);
  Tape *tape = tape_create();
  produce_instructions(etree, tape);
  // tape_write(tape, stdout);
  ModuleInfo *module_info = _modulemanager_hydrate(mm, tape);
  module_info->fi = fi;
  delete_expression(etree);
  syntax_tree_delete(&stree);
  return &module_info->module;
}

Module *modulemanager_read(ModuleManager *mm, const char fn[]) {
  Module *module = _read_helper(mm, fn);
  module->_reflection = heap_new(mm->_heap, Class_Module);
  _add_reflection(mm, module);
  return module;
}
