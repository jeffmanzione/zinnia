// vm.h
//
// Created on: Jan 1, 2021
//     Author: Jeff Manzione

#include "struct/alist.h"
#include "util/sync/mutex.h"
#include "vm/module_manager.h"
#include "vm/process/processes.h"

typedef struct _VM {
  ModuleManager mm;

  AList processes;
  Mutex process_create_lock;
  Process *main;
} VM;

ModuleManager *vm_module_manager(VM *vm);

Process *vm_create_process(VM *vm);