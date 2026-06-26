// builtin_modules.h
//
// Created on: Jan 1, 2021
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_BUILTIN_MODULES_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_BUILTIN_MODULES_H_

#include "zinnia/heap/heap.h"
#include "zinnia/vm/module_manager.h"

void register_builtin(ModuleManager *mm, Heap *heap, const char *lib_location);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_BUILTIN_MODULES_H_ */