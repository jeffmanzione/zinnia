// virtual_machine.h
//
// Created on: Jun 13, 2020
//     Author: Jeff Manzione

#ifndef VM_VIRTUAL_MACHINE_H_
#define VM_VIRTUAL_MACHINE_H_

#include "util/sync/thread.h"
#include "vm/module_manager.h"
#include "vm/process/processes.h"
#include "vm/vm.h"

VM *vm_create(const char *lib_location, uint32_t max_object_count,
              bool async_enabled);
void vm_delete(VM *vm);

Process *vm_create_process(VM *vm);
Process *vm_main_process(VM *vm);

void process_run(Process *process);
ThreadHandle process_run_in_new_thread(Process *process);

#endif /* VM_VIRTUAL_MACHINE_H_ */