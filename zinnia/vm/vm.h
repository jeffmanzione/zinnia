// vm.h
//
// Created on: Jan 1, 2021
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_VM_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_VM_H_

#include "c-data-structures/arraylike.h"
#include "zinnia/util/sync/mutex.h"
#include "zinnia/util/sync/threadpool.h"
#include "zinnia/vm/isolate/isolates.h"
#include "zinnia/vm/module_manager.h"

DEFINE_ARRAYLIKE(IsolateArray, Isolate);

typedef struct _VM {
  ModuleManager mm;

  IsolateArray isolates;
  Mutex isolate_create_lock;
  Isolate *main;
  ThreadPool *blocking_pool;
  HeapConf base_heap_conf;
  bool async_enabled;
  // Exit code of the program. Set by `exit` or an uncaught error.
  volatile int exit_code;
} VM;

ModuleManager *vm_module_manager(VM *vm);
Isolate *vm_create_isolate(VM *vm);
Isolate *create_isolate_no_reflection(VM *vm);
void add_reflection_to_isolate(Isolate *isolate);
Entity object_get_maybe_wrap(Object *obj, const char field[], Heap *heap,
                             Scope *ctx);

uint32_t isolate_collect_garbage(Isolate *isolate);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_VM_H_ */
