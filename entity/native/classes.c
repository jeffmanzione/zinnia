// classes.c
//
// Created on: Jan 1, 2021
//     Author: Jeff Manzione

#include "entity/native/classes.h"
#include "alloc/arena/intern.h"
#include "entity/class/classes.h"
#include "entity/entity.h"
#include "entity/native/error.h"
#include "entity/native/native.h"
#include "entity/string/string.h"
#include "entity/tuple/tuple.h"
#include "lang/parser/lang_parser.h"
#include "lang/parser/parser.h"
#include "lang/semantic_analyzer/definitions.h"
#include "lang/semantic_analyzer/expression_tree.h"
#include "lang/semantic_analyzer/semantic_analyzer.h"
#include "struct/map.h"
#include "struct/struct_defaults.h"
#include "util/file/file_info.h"
#include "vm/module_manager.h"
#include "vm/process/processes.h"
#include "vm/vm.h"

Entity _load_class_from_text(Task *task, Context *ctx, Object *obj,
                             Entity *args) {
  if (!IS_TUPLE(args)) {
    return raise_error(task, ctx,
                       "Invalid arguments for __load_class_from_text: Input is "
                       "wrong type. Expected tuple(2).");
  }
  const Tuple *t = (Tuple *)args->obj->_internal_obj;
  if (2 != tuple_size(t)) {
    return raise_error(
        task, ctx,
        "Invalid number of arguments for "
        "__load_class_from_text: Expected 2 arguments but got %d.",
        tuple_size(t));
  }

  const Entity *arg0 = tuple_get(t, 0);
  const Entity *arg1 = tuple_get(t, 1);
  if (!IS_CLASS(arg0, Class_Module)) {
    return raise_error(task, ctx,
                       "Invalid argument(0) for "
                       "__load_class_from_text: Expected type Module.");
  }
  Module *m = arg0->obj->_module_obj;

  if (!IS_CLASS(arg1, Class_String)) {
    return raise_error(task, ctx,
                       "Invalid argument(1) for "
                       "__load_class_from_text: Expected type String.");
  }
  String *class_text = (String *)arg1->obj->_internal_obj;

  char *c_str_text = strndup(class_text->table, String_size(class_text));
  SFILE *file = sfile_open(c_str_text);

  Tape *tape = (Tape *)m->_tape; // bless
  FileInfo *fi = file_info_sfile(file);
  FileInfo *module_fi = (FileInfo *)modulemanager_get_fileinfo(
      vm_module_manager(task->parent_process->vm), m);
  file_info_append(module_fi, fi);
  fi = module_fi;

  Q tokens;
  Q_init(&tokens);

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

  Q_finalize(&tokens);

  Map new_classes;
  map_init_default(&new_classes);
  modulemanager_update_module(vm_module_manager(task->parent_process->vm), m,
                              &new_classes);

  if (1 != map_size(&new_classes)) {
    return raise_error(task, ctx, "Weird error adding a new class.");
  }

  M_iter iter = map_iter(&new_classes);
  Class *new_class = (Class *)value(&iter);

  map_finalize(&new_classes);

  DEALLOC(c_str_text);

  return entity_object(new_class->_reflection);
}

void classes_add_native(ModuleManager *mm, Module *classes) {
  native_function(classes, intern("__load_class_from_text"),
                  _load_class_from_text);
}