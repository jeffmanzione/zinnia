// run.c
//
// Created on: Oct 30, 2020
//     Author: Jeff Manzione

#include "zinnia/run/run.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "file-utils/file_info.h"
#include "file-utils/string_utils.h"
#include "language-tools/intern.h"
#include "language-tools/parser/parser.h"
#include "language-tools/semantic_analyzer/expression_tree.h"
#include "language-tools/semantic_analyzer/semantic_analyzer.h"
#include "zinnia/alloc/alloc.h"
#include "zinnia/compile/compile.h"
#include "zinnia/entity/class/classes_def.h"
#include "zinnia/entity/entity.h"
#include "zinnia/entity/module/modules.h"
#include "zinnia/entity/object.h"
#include "zinnia/entity/string/string_helper.h"
#include "zinnia/program/optimization/optimize.h"
#include "zinnia/program/tape.h"
#include "zinnia/seed/seed.h"
#include "zinnia/util/args/commandline.h"
#include "zinnia/util/args/commandlines.h"
#include "zinnia/util/args/lib_finder.h"
#include "zinnia/util/file.h"
#include "zinnia/util/sync/constants.h"
#include "zinnia/util/sync/thread.h"
#include "zinnia/vm/intern.h"
#include "zinnia/vm/module_manager.h"
#include "zinnia/vm/process/process.h"
#include "zinnia/vm/process/processes.h"
#include "zinnia/vm/process/task.h"
#include "zinnia/vm/virtual_machine.h"

IMPL_ARRAYLIKE(FilePartsArray, FileParts);

void set_args_(Heap *heap, ArgStore *store) {
  Object *args = heap_new(heap, Class_Object);
  ArgMapIterator cl_args;
  ArgMap_iterator(&cl_args, argstore_program_args(store));
  for (; ArgMap_has_entry(&cl_args); ArgMap_next_entry(&cl_args)) {
    const char *k = ArgMap_key(&cl_args);
    const char *v = *ArgMap_value(&cl_args);
    object_set_member_obj(heap, args, k,
                          istring_new_no_intern(heap, v, strlen(v)));
  }
  object_set_member_obj(heap, Module_builtin->_reflection,
                        global_intern("args"), args);
}

void run_files(const CharPtrArray *source_file_names,
               const FilePartsArray *source_contents,
               const VoidPtrArray *init_fns, ArgStore *store) {
  optimize_init();

  const char *lib_location =
      argstore_lookup_string(store, ArgKey__LIB_LOCATION);
  uint32_t max_process_object_count =
      argstore_lookup_int(store, ArgKey__MAX_PROCESS_OBJECT_COUNT);
  bool async_enabled = argstore_lookup_bool(store, ArgKey__ASYNC);
  VM *vm = vm_create(lib_location, max_process_object_count, async_enabled);
  ModuleManager *mm = vm_module_manager(vm);
  Module *main_module = NULL;

  for (int i = 0; i < CharPtrArray_size(source_file_names); ++i) {
    const char *src = CharPtrArray_get_unchecked(source_file_names, i);
    const FileParts *src_content =
        FilePartsArray_get_ref_unchecked(source_contents, i);

    if (src_content->type == FP_FILE_SEED) {
      ASSERT(src_content->seed_data.content_size > 0);
      TmpSeedFile tmp_file;
      char error_buf[255];
      copy_znseed_bytes_to_tmpfile(src_content->seed_data.content_bytes,
                                   src_content->seed_data.content_size,
                                   &tmp_file);
      if (!load_znseed_file(vm, tmp_file.filename, error_buf)) {
        FATALF("%s", error_buf);
      }
      znseed_tmp_finalize(&tmp_file);
    } else {
      NativeModuleInitFn init_fn =
          (NativeModuleInitFn)VoidPtrArray_get_unchecked(init_fns, i);
      ModuleInfo *module_info = mm_register_module_with_callback(
          mm, src, src, src_content->source.parts,
          src_content->source.num_parts, init_fn);

      if (NULL == main_module) {
        main_module = modulemanager_load(mm, module_info);
        main_module->_is_initialized = true;
        object_set_member(vm_main_process(vm)->heap, main_module->_reflection,
                          MAIN_KEY, &TRUE_ENTITY);
      }
    }
  }

  set_args_(vm_main_process(vm)->heap, store);
  Task *task = process_create_task(vm_main_process(vm));
  task_create_context(task, main_module->_reflection, main_module, 0);
  process_run(vm_main_process(vm));

#ifdef DEBUG
  vm_delete(vm);
  optimize_finalize();
#endif
}

void run(const SourceNameSet *source_files, ArgStore *store) {
  optimize_init();

  const char *lib_location =
      argstore_lookup_string(store, ArgKey__LIB_LOCATION);
  uint32_t max_process_object_count =
      argstore_lookup_int(store, ArgKey__MAX_PROCESS_OBJECT_COUNT);
  bool async_enabled = argstore_lookup_bool(store, ArgKey__ASYNC);
  VM *vm = vm_create(lib_location, max_process_object_count, async_enabled);
  ModuleManager *mm = vm_module_manager(vm);
  Module *main_module = NULL;

  SourceNameSetIterator srcs;
  SourceNameSet_iterator(&srcs, source_files);
  for (; SourceNameSet_has_next(&srcs); SourceNameSet_next(&srcs)) {
    const char *src = *SourceNameSet_value(&srcs);
    if (is_dir(src)) {
      FileLocs *locs = file_locs_create(src);
      FileLoc_iter iter = file_locs_iter(locs);
      for (; fl_has(&iter); fl_inc(&iter)) {
        FileLoc *loc = fl_value(&iter);
        const char *full_path = file_loc_full_path(loc);
        if (ends_with(full_path, ZNSEED_EXTENSION)) {
          char error_buf[255];
          if (!load_znseed_file(vm, full_path, error_buf)) {
            FATALF("%s", error_buf);
          }
        } else {
          mm_register_module(mm, full_path, file_loc_relative_path(loc), NULL,
                             -1);
        }
      }
      file_locs_delete(locs);
    } else {
      if (ends_with(src, ZNSEED_EXTENSION)) {
        char error_buf[255];
        if (!load_znseed_file(vm, src, error_buf)) {
          FATALF("%s", error_buf);
        }
      } else {
        ModuleInfo *module_info = mm_register_module(mm, src, src, NULL, -1);
        if (NULL == main_module) {
          main_module = modulemanager_load(mm, module_info);
          main_module->_is_initialized = true;
          object_set_member(vm_main_process(vm)->heap, main_module->_reflection,
                            MAIN_KEY, &TRUE_ENTITY);
        }
      }
    }
  }

  set_args_(vm_main_process(vm)->heap, store);
  Task *task = process_create_task(vm_main_process(vm));
  task_create_context(task, main_module->_reflection, main_module, 0);
  process_run(vm_main_process(vm));

#ifdef DEBUG
  vm_delete(vm);
  optimize_finalize();
#endif
}

int zinnia(int argc, const char *argv[]) {
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
#endif

  return EXIT_SUCCESS;
}