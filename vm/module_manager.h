// module_manager.h
//
// Created on: Jul 10, 2020
//     Author: Jeff Manzione

#ifndef VM_MODULE_MANAGER_H_
#define VM_MODULE_MANAGER_H_

#include "entity/module/module.h"
#include "heap/heap.h"
#include "program/tape.h"
#include "struct/keyed_list.h"
#include "struct/set.h"
#include "util/file/file_info.h"

typedef char *(*InternFn)(const char *);

typedef struct {
  Set _files_processed;
  Heap *_heap;
  KeyedList _modules; // ModuleInfo
  // Used to intern method names in dynamically-loaded modules.
  InternFn intern;
} ModuleManager;

typedef struct _ModuleInfo ModuleInfo;
typedef void (*NativeCallback)(ModuleManager *, Module *);

void modulemanager_init(ModuleManager *mm, Heap *heap);
void modulemanager_finalize(ModuleManager *mm);
Module *modulemanager_load(ModuleManager *mm, ModuleInfo *module_info);

ModuleInfo *mm_register_module(ModuleManager *mm, const char full_path[],
                               const char relative_path[],
                               const char *inline_file_segs[],
                               int num_inlined_file_segs);
ModuleInfo *mm_register_module_with_callback(ModuleManager *mm,
                                             const char full_path[],
                                             const char relative_path[],
                                             const char *inlined_file_segs[],
                                             int num_inlined_file_segs,
                                             NativeCallback callback);
ModuleInfo *mm_register_dynamic_module(ModuleManager *mm,
                                       const char module_name[],
                                       NativeCallback init_fn);

Module *modulemanager_lookup(ModuleManager *mm, const char module_key[]);
Module *modulemanager_lookup_without_reading(ModuleManager *mm,
                                             const char module_name[]);

const FileInfo *modulemanager_get_fileinfo(const ModuleManager *mm,
                                           const Module *m);

void add_reflection_to_module(ModuleManager *mm, Module *module);
void add_reflection_to_function(Heap *heap, Object *parent, Function *func);
void modulemanager_update_module(ModuleManager *mm, Module *m,
                                 Map *new_classes);

const char *module_info_file_name(ModuleInfo *mi);

void fatal_on_token(const char file_name[], FileInfo *fi, Q *tokens);

#endif /* VM_MODULE_MANAGER_H_ */