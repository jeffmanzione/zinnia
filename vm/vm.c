// vm.c
//
// Created on: Jan 1, 2021
//     Author: Jeff Manzione

#include "vm/vm.h"

inline ModuleManager *vm_module_manager(VM *vm) { return &vm->mm; }