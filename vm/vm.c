// vm.c
//
// Created on: Jan 1, 2021
//     Author: Jeff Manzione

#include "vm/vm.h"

#include "entity/class/classes.h"
#include "vm/process/process.h"

Process *create_process_no_reflection(VM *vm) {
  Process *process = alist_add(&vm->processes);
  process_init(process);
  process->vm = vm;
  return process;
}

void add_reflection_to_process(Process *process) {
  process->_reflection = heap_new(process->heap, Class_Process);
  process->_reflection->_internal_obj = process;
  heap_make_root(process->heap, process->_reflection);
}

inline ModuleManager *vm_module_manager(VM *vm) { return &vm->mm; }

Process *vm_create_process(VM *vm) {
  Process *process;
  SYNCHRONIZED(vm->process_create_lock, {
    process = create_process_no_reflection(vm);
    add_reflection_to_process(process);
  });
  return process;
}