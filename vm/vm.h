// vm.h
//
// Created on: Jan 1, 2021
//     Author: Jeff Manzione

#ifndef VM_VM_H_
#define VM_VM_H_

#include "struct/alist.h"
#include "util/sync/mutex.h"
#include "util/sync/threadpool.h"
#include "vm/module_manager.h"
#include "vm/process/processes.h"

typedef struct _VM {
  ModuleManager mm;

  AList processes;
  Mutex process_create_lock;
  Process *main;
  ThreadPool *background_pool;
} VM;

ModuleManager *vm_module_manager(VM *vm);
Process *vm_create_process(VM *vm);
Process *create_process_no_reflection(VM *vm);
void add_reflection_to_process(Process *process);

#endif /* VM_VM_H_ */