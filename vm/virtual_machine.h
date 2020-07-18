// virtual_machine.h
//
// Created on: Jun 13, 2020
//     Author: Jeff Manzione

#ifndef VM_VIRTUAL_MACHINE_H_
#define VM_VIRTUAL_MACHINE_H_

#include "vm/module_manager.h"
#include "vm/process/processes.h"

typedef struct _VM VM;

VM *vm_create();
void vm_delete(VM *vm);

Process *vm_main_process(VM *vm);

TaskState vm_execute_task(VM *vm, Task *task);

Process *vm_create_process(VM *vm);

ModuleManager *vm_module_manager(VM *vm);

#endif /* VM_VIRTUAL_MACHINE_H_ */