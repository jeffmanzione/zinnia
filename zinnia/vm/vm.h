// vm.h
//
// Created on: Jan 1, 2021
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_VM_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_VM_H_

#include "c-data-structures/arraylike.h"
#include "zinnia/util/sync/mutex.h"
#include "zinnia/util/sync/threadpool.h"
#include "zinnia/vm/module_manager.h"
#include "zinnia/vm/process/processes.h"

DEFINE_ARRAYLIKE(ProcessArray, Process);

typedef struct _VM {
  ModuleManager mm;

  ProcessArray processes;
  Mutex process_create_lock;
  Process *main;
  ThreadPool *background_pool;
  HeapConf base_heap_conf;
  bool async_enabled;
} VM;

ModuleManager *vm_module_manager(VM *vm);
Process *vm_create_process(VM *vm);
Process *create_process_no_reflection(VM *vm);
void add_reflection_to_process(Process *process);
Entity object_get_maybe_wrap(Object *obj, const char field[], Heap *heap,
                             Context *ctx);

uint32_t process_collect_garbage(Process *process);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_VM_H_ */