// virtual_machine.h
//
// Created on: Jun 13, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_VIRTUAL_MACHINE_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_VIRTUAL_MACHINE_H_

#include "zinnia/util/sync/thread.h"
#include "zinnia/vm/isolate/isolates.h"
#include "zinnia/vm/module_manager.h"
#include "zinnia/vm/vm.h"

VM *vm_create(const char *lib_location, uint32_t max_object_count,
              bool async_enabled);
void vm_delete(VM *vm);

Isolate *vm_create_isolate(VM *vm);
Isolate *vm_main_isolate(VM *vm);

void isolate_run(Isolate *isolate);
ThreadHandle isolate_run_in_new_thread(Isolate *isolate);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_VIRTUAL_MACHINE_H_ */