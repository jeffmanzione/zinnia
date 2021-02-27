// builtin_modules.h
//
// Created on: Jan 1, 2021
//     Author: Jeff Manzione

#ifndef VM_BUILTIN_MODULES_H_
#define VM_BUILTIN_MODULES_H_

#include "heap/heap.h"
#include "vm/module_manager.h"

void read_builtin(ModuleManager *mm, Heap *heap, const char *lib_location);

#endif /* VM_BUILTIN_MODULES_H_ */