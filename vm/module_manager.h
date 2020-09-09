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

extern Module *Module_builtin;
extern Module *Module_io;

typedef struct {
  Heap *_heap;
  KeyedList _modules; // ModuleInfo
} ModuleManager;

void modulemanager_init(ModuleManager *mm, Heap *heap);
void modulemanager_finalize(ModuleManager *mm);
Module *modulemanager_read(ModuleManager *mm, const char fn[]);
Module *modulemanager_lookup(ModuleManager *mm, const char fn[]);

#endif /* VM_MODULE_MANAGER_H_ */