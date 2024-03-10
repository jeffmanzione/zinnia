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
#include "lang/parser/parser.h"
#include "lang/semantic_analyzer/expression_tree.h"
#include "lang/semantic_analyzer/semantic_analyzer.h"
#include "program/optimization/optimize.h"
#include "program/tape.h"
#include "struct/alist.h"
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

void run_files(const AList *source_file_names, const AList *source_contents,
               const AList *init_fns, ArgStore *store) {
  optimize_init();

  const char *lib_location =
      argstore_lookup_string(store, ArgKey__LIB_LOCATION);
  uint32_t max_process_object_count =
      argstore_lookup_int(store, ArgKey__MAX_PROCESS_OBJECT_COUNT);
  bool async_enabled = argstore_lookup_bool(store, ArgKey__ASYNC);
  VM *vm = vm_create(lib_location, max_process_object_count, async_enabled);
  ModuleManager *mm = vm_module_manager(vm);
  Module *main_module = NULL;

  for (int i = 0; i < alist_len(source_file_names); ++i) {
    const char *src = *(char **)alist_get(source_file_names, i);
    const char *src_content = *(char **)alist_get(source_contents, i);
    NativeCallback init_fn = *(NativeCallback *)alist_get(init_fns, i);
    ModuleInfo *module_info =
        mm_register_module_with_callback(mm, src, src_content, init_fn);
    if (NULL == main_module) {
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

void run(const Set *source_files, ArgStore *store) {
  optimize_init();

  const char *lib_location =
      argstore_lookup_string(store, ArgKey__LIB_LOCATION);
  uint32_t max_process_object_count =
      argstore_lookup_int(store, ArgKey__MAX_PROCESS_OBJECT_COUNT);
  bool async_enabled = argstore_lookup_bool(store, ArgKey__ASYNC);
  VM *vm = vm_create(lib_location, max_process_object_count, async_enabled);
  ModuleManager *mm = vm_module_manager(vm);
  Module *main_module = NULL;

  M_iter srcs = set_iter((Set *)source_files);
  for (; has(&srcs); inc(&srcs)) {
    const char *src = value(&srcs);
    ModuleInfo *module_info = mm_register_module(mm, src, NULL);
    if (NULL == main_module) {
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

int zinnia(int argc, const char *argv[]) {
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