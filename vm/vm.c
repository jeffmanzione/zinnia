// vm.c
//
// Created on: Jan 1, 2021
//     Author: Jeff Manzione

#include "vm/vm.h"

inline ModuleManager *vm_module_manager(VM *vm) { return &vm->mm; }

Process *vm_create_process(VM *vm) {
  Process *process;
  SYNCHRONIZED(vm->process_create_lock, {
    process = _create_process_no_reflection(vm);
    _add_reflection_to_process(process);
  });
  return process;
}