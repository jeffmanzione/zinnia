// run.c
//
// Created on: Oct 30, 2020
//     Author: Jeff Manzione

#include "run/run.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "alloc/alloc.h"
#include "compile/compile.h"
#include "entity/object.h"
#include "lang/lexer/file_info.h"
#include "lang/parser/parser.h"
#include "lang/semantics/expression_tree.h"
#include "lang/semantics/semantics.h"
#include "program/optimization/optimize.h"
#include "program/tape.h"
#include "struct/map.h"
#include "struct/set.h"
#include "util/args/commandline.h"
#include "util/args/commandlines.h"
#include "util/args/lib_finder.h"
#include "util/file.h"
#include "util/string.h"
#include "util/sync/constants.h"
#include "util/sync/thread.h"
#include "vm/intern.h"
#include "vm/module_manager.h"
#include "vm/process/process.h"
#include "vm/process/processes.h"
#include "vm/process/task.h"
#include "vm/virtual_machine.h"

void run(const Set *source_files, ArgStore *store) {
  const bool out_ja = argstore_lookup_bool(store, ArgKey__OUT_ASSEMBLY);
  const char *machine_dir =
      argstore_lookup_string(store, ArgKey__ASSEMBLY_OUT_DIR);
  const bool out_jb = argstore_lookup_bool(store, ArgKey__OUT_BINARY);
  const char *bytecode_dir = argstore_lookup_string(store, ArgKey__BIN_OUT_DIR);

  parsers_init();
  semantics_init();
  optimize_init();

  VM *vm = vm_create();
  ModuleManager *mm = vm_module_manager(vm);
  Module *main_module = NULL;

  M_iter srcs = set_iter((Set *)source_files);
  for (; has(&srcs); inc(&srcs)) {
    const char *src = value(&srcs);
    Module *module = modulemanager_read(mm, src);

    write_tape(src, module_tape(module), out_ja, machine_dir, out_jb,
               bytecode_dir);

    if (NULL == main_module) {
      main_module = module;
      heap_make_root(vm_main_process(vm)->heap, main_module->_reflection);
    }
  }

  optimize_finalize();
  semantics_finalize();
  parsers_finalize();

  Task *task = process_create_task(vm_main_process(vm));
  task_create_context(task, main_module->_reflection, main_module, 0);
  process_run(vm_main_process(vm));

  vm_delete(vm);
}

int jlr(int argc, const char *argv[]) {
  alloc_init();
  strings_init();

  ArgConfig *config = argconfig_create();
  argconfig_compile(config);
  argconfig_run(config);
  ArgStore *store = commandline_parse_args(config, argc, argv);

  run(argstore_sources(store), store);

  argstore_delete(store);
  argconfig_delete(config);

  strings_finalize();
  token_finalize_all();
  alloc_finalize();

  return EXIT_SUCCESS;
}