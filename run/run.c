// run.c
//
// Created on: Oct 30, 2020
//     Author: Jeff Manzione

#include "run/run.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "alloc/alloc.h"
#include "alloc/arena/intern.h"
#include "compile/compile.h"
#include "entity/class/classes_def.h"
#include "entity/entity.h"
#include "entity/module/modules.h"
#include "entity/object.h"
#include "entity/string/string_helper.h"
#include "lang/lexer/lang_lexer.h"
#include "lang/lexer/token.h"
#include "lang/parser/lang_parser.h"
#include "lang/parser/parser.h"
#include "lang/semantic_analyzer/definitions.h"
#include "lang/semantic_analyzer/expression_tree.h"
#include "lang/semantic_analyzer/semantic_analyzer.h"
#include "program/optimization/optimize.h"
#include "program/tape.h"
#include "struct/map.h"
#include "struct/set.h"
#include "util/args/commandline.h"
#include "util/args/commandlines.h"
#include "util/args/lib_finder.h"
#include "util/file.h"
#include "util/file/file_info.h"
#include "util/string.h"
#include "util/sync/constants.h"
#include "util/sync/thread.h"
#include "vm/intern.h"
#include "vm/module_manager.h"
#include "vm/process/process.h"
#include "vm/process/processes.h"
#include "vm/process/task.h"
#include "vm/virtual_machine.h"

void _set_args(Heap *heap, ArgStore *store) {
  Object *args = heap_new(heap, Class_Object);
  M_iter cl_args = map_iter((Map *)argstore_program_args(store));
  for (; has(&cl_args); inc(&cl_args)) {
    const char *k = key(&cl_args);
    const char *v = value(&cl_args);
    object_set_member_obj(heap, args, k, string_new(heap, v, strlen(v)));
  }
  object_set_member_obj(heap, Module_builtin->_reflection, intern("args"),
                        args);
}

void _interpret_and_execute_line(Module *m) {
  char buf[4096];
  getline();

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
  parser_init(&parser, rule_file_level_statement,
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
}

void run(const Set *source_files, ArgStore *store) {
  optimize_init();

  const char *lib_location =
      argstore_lookup_string(store, ArgKey__LIB_LOCATION);
  const uint32_t max_process_object_count =
      argstore_lookup_int(store, ArgKey__MAX_PROCESS_OBJECT_COUNT);
  const bool async_enabled = argstore_lookup_bool(store, ArgKey__ASYNC);
  VM *vm = vm_create(lib_location, max_process_object_count, async_enabled);
  ModuleManager *mm = vm_module_manager(vm);
  Module *main_module = NULL;

  M_iter srcs = set_iter((Set *)source_files);
  const bool has_sources = set_size(source_files) > 0;
  const bool interpreter_mode =
      argstore_lookup_bool(store, ArgKey__INTERPRETER) || !has_sources;

  if (!has_sources && !interpreter_mode) {
    fprintf(
        stderr,
        "Error: No sources provided. If interepter mode was desired, add -i.");
    return;
  }
  if (interpreter_mode) {
    fprintf(stdout, "Interpreter mode detected.\n");
  }

  for (; has(&srcs); inc(&srcs)) {
    const char *src = value(&srcs);
    ModuleInfo *module_info = mm_register_module(mm, src, NULL);
    if (!interpreter_mode && NULL == main_module) {
      main_module = modulemanager_load(mm, module_info);
      main_module->_is_initialized = true;
      object_set_member(vm_main_process(vm)->heap, main_module->_reflection,
                        MAIN_KEY, &TRUE_ENTITY);
    }
  }

  _set_args(vm_main_process(vm)->heap, store);
  Task *task = process_create_task(vm_main_process(vm));
  task_create_context(task, main_module->_reflection, main_module, 0);
  process_run(vm_main_process(vm));

#ifdef DEBUG
  vm_delete(vm);
  optimize_finalize();
#endif
}

int jasper(int argc, const char *argv[]) {
  alloc_init();
  strings_init();

  ArgConfig *config = argconfig_create();
  argconfig_run(config);
  ArgStore *store = commandline_parse_args(config, argc, argv);

  run(argstore_sources(store), store);

#ifdef DEBUG
  argstore_delete(store);
  argconfig_delete(config);

  strings_finalize();
  token_finalize_all();
  alloc_finalize();
#endif

  return EXIT_SUCCESS;
}