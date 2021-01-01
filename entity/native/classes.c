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
#include "lang/parser/parser.h"
#include "lang/semantics/expression_tree.h"
#include "lang/semantics/semantics.h"
#include "struct/map.h"
#include "struct/struct_defaults.h"
#include "util/file/file_info.h"
#include "vm/module_manager.h"
#include "vm/process/processes.h"
#include "vm/vm.h"

Entity _load_class_from_text(Task *task, Context *ctx, Object *obj,
                             Entity *args) {
  if (NULL == args || args->type != OBJECT ||
      Class_Tuple != args->obj->_class) {
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
  if (NULL == arg0 || OBJECT != arg0->type ||
      Class_Module != arg0->obj->_class) {
    return raise_error(task, ctx,
                       "Invalid argument(0) for "
                       "__load_class_from_text: Expected type Module.");
  }
  Module *m = arg0->obj->_module_obj;

  if (NULL == arg1 || OBJECT != arg1->type ||
      Class_String != arg1->obj->_class) {
    return raise_error(task, ctx,
                       "Invalid argument(1) for "
                       "__load_class_from_text: Expected type String.");
  }
  String *class_text = (String *)arg1->obj->_internal_obj;

  parsers_init();
  semantics_init();

  char *c_str_text = ALLOC_ARRAY2(char, String_size(class_text) + 1);
  memmove(c_str_text, class_text->table, String_size(class_text));
  c_str_text[String_size(class_text)] = '\0';

  SFILE *file = sfile_open(c_str_text);

  FileInfo *fi = file_info_sfile(file);
  SyntaxTree stree = parse_file(fi);
  ExpressionTree *etree = populate_expression(&stree);

  Tape *tape = (Tape *)m->_tape; // bless
  produce_instructions(etree, tape);
  delete_expression(etree);
  syntax_tree_delete(&stree);

  tape_write(tape, stdout);

  DEBUGF("A");

  Map new_classes;
  map_init_default(&new_classes);
  modulemanager_update_module(vm_module_manager(task->parent_process->vm), m,
                              &new_classes);

  DEBUGF("B");
  if (1 != map_size(&new_classes)) {
    return raise_error(task, ctx, "Weird error adding a new class.");
  }

  M_iter iter = map_iter(&new_classes);
  Class *new_class = (Class *)value(&iter);

  map_finalize(&new_classes);

  semantics_finalize();
  parsers_finalize();

  return entity_object(new_class->_reflection);
}

void classes_add_native(Module *classes) {
  native_function(classes, intern("__load_class_from_text"),
                  _load_class_from_text);
}