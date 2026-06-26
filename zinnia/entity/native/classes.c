// classes.c
//
// Created on: Jan 1, 2021
//     Author: Jeff Manzione

#include "zinnia/entity/native/classes.h"

#include "file-utils/file_info.h"
#include "language-tools/intern.h"
#include "language-tools/parser/parser.h"
#include "language-tools/semantic_analyzer/expression_tree.h"
#include "language-tools/semantic_analyzer/semantic_analyzer.h"
#include "zinnia/alloc/alloc.h"
#include "zinnia/entity/class/classes_def.h"
#include "zinnia/entity/entity.h"
#include "zinnia/entity/native/error.h"
#include "zinnia/entity/native/native.h"
#include "zinnia/entity/native/native_helpers.h"
#include "zinnia/entity/string/string.h"
#include "zinnia/entity/string/string_helper.h"
#include "zinnia/entity/tuple/tuple.h"
#include "zinnia/lang/parser/lang_parser.h"
#include "zinnia/lang/semantic_analyzer/definitions.h"
#include "zinnia/util/platform.h"
#include "zinnia/vm/module_manager.h"
#include "zinnia/vm/process/processes.h"
#include "zinnia/vm/vm.h"

#ifndef OS_WINDOWS
#include "zinnia/lang/lexer/lang_lexer.h"
#endif

Entity load_class_from_text_(Task *task, Context *ctx, Object *obj,
                             Entity *args) {
  EXTRACT_TUPLE_ARGS(t, args, 2, task, ctx);

  const Entity *arg0 = tuple_get(t, 0);
  const Entity *arg1 = tuple_get(t, 1);
  if (!IS_CLASS(arg0, Class_Module)) {
    return raise_error(task, ctx,
                       "Invalid argument(0) for "
                       "__load_class_from_text: Expected type Module.");
  }
  Module *m = arg0->obj->_module_obj;

  if (!IS_STRING(arg1)) {
    return raise_error(task, ctx,
                       "Invalid argument(1) for "
                       "__load_class_from_text: Expected type String.");
  }

  char *c_str_text = entity_string_copy(arg1);
  SFILE *file = sfile_open(c_str_text);

  Tape *tape = (Tape *)m->_tape;  // bless
  FileInfo *fi = file_info_sfile(file);
  FileInfo *module_fi = (FileInfo *)modulemanager_get_fileinfo(
      vm_module_manager(task->parent_process->vm), m);
  file_info_append(module_fi, fi);
  fi = module_fi;

  TokenArray tokens;
  TokenArray_init(&tokens);

  lexer_tokenize(fi, &tokens);

  Parser parser;
  parser_init(&parser, rule_file_level_statement_list,
              /*ignore_newline=*/false);
  SyntaxTree *stree = parser_parse(&parser, &tokens);
  stree = parser_prune_newlines(&parser, stree);

  SemanticAnalyzer sa;
  semantic_analyzer_init(&sa, semantic_analyzer_init_fn);
  ExpressionTree *etree = semantic_analyzer_populate(&sa, stree);

  semantic_analyzer_produce(&sa, etree, tape);

  semantic_analyzer_delete(&sa, etree);
  semantic_analyzer_finalize(&sa);

  parser_delete_st(&parser, stree);
  parser_finalize(&parser);

  TokenArray_finalize(&tokens);

  ClassPtrMap new_classes;
  ClassPtrMap_init(&new_classes, hash_interned_string,
                   compare_interned_strings);
  modulemanager_update_module(vm_module_manager(task->parent_process->vm), m,
                              &new_classes);

  if (1 != ClassPtrMap_size(&new_classes)) {
    RELEASE(c_str_text);
    return raise_error(task, ctx, "Weird error adding a new class.");
  }

  ClassPtrMapIterator iter;
  ClassPtrMap_iterator(&iter, &new_classes);
  Class *new_class = *ClassPtrMap_mutable_value(&iter);

  ClassPtrMap_finalize(&new_classes);

  RELEASE(c_str_text);

  return entity_object(new_class->_reflection);
}

void classes_add_native(ModuleManager *mm, Module *classes) {
  native_function(classes, global_intern("__load_class_from_text"),
                  load_class_from_text_);
}