// module_manager.h
//
// Created on: Jul 10, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_MODULE_MANAGER_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_MODULE_MANAGER_H_

#include "c-data-structures/maplike.h"
#include "c-data-structures/stable_maplike.h"
#include "file-utils/file_info.h"
#include "zinnia/entity/module/module.h"
#include "zinnia/heap/heap.h"
#include "zinnia/program/tape.h"

typedef const char *(*InternFn)(const char *);

typedef struct ModuleInfoMap_ ModuleInfoMap;

typedef struct {
  // Set _files_processed;
  Heap *_heap;
  ModuleInfoMap *_modules;  // ModuleInfo
  // Used to intern method names in dynamically-loaded modules.
  InternFn intern;
} ModuleManager;

typedef struct ModuleInfo_ ModuleInfo;
typedef void (*NativeModuleInitFn)(ModuleManager *, Module *);

// Compile-time/runtime check of init function conformance.
bool verify_init_fn_signature(NativeModuleInitFn fn);

DEFINE_STABLE_MAPLIKE(ModuleInfoMap, char *, ModuleInfo);
DEFINE_MAPLIKE(ClassPtrMap, char *, Class *);

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
                                             NativeModuleInitFn callback);
ModuleInfo *mm_register_dynamic_module(ModuleManager *mm,
                                       const char module_name[],
                                       NativeModuleInitFn init_fn);

Module *modulemanager_lookup(ModuleManager *mm, const char module_key[]);
Module *modulemanager_lookup_without_reading(ModuleManager *mm,
                                             const char module_name[]);

const FileInfo *modulemanager_get_fileinfo(const ModuleManager *mm,
                                           const Module *m);

void add_reflection_to_module(ModuleManager *mm, Module *module);
void add_reflection_to_function(Heap *heap, Object *parent, Function *func);
void modulemanager_update_module(ModuleManager *mm, Module *m,
                                 ClassPtrMap *new_classes);

const char *module_info_file_name(ModuleInfo *mi);

void fatal_on_token(const char file_name[], FileInfo *fi, TokenArray *tokens);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_MODULE_MANAGER_H_ */