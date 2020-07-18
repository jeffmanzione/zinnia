// thread.h
//
// Created on: Jun 13, 2020
//     Author: Jeff Manzione

#ifndef VM_PROCESS_THREAD_H_
#define VM_PROCESS_THREAD_H_

#include "struct/alist.h"
#include "struct/set.h"
#include "vm/virtual_machine.h"

typedef enum {
  // Waiting to acquire a lock.
  BLOCKED,
  // Not begun execution.
  NEW,
  // Thread is currently running or will run when it gains access to the CPU.
  RUNNABLE,
  // Has completed execution.
  TERMINATED,
  // Suspended execution for a specified amount of time, e.g., with sleep().
  TIMED_WAITING,
  // Suspended execution and waiting for some action to occur.
  WAITING
} ThreadState;

typedef struct {
  volatile ThreadState state;
  VM *vm;
  AList contexts;
  Set locks;
} VMThread;

void vmthread_init(VMThread *thread, VM *vm);
void vmthread_start(VMThread *thread, Context *ctx);
void vmthread_finalize(VMThread *thread);

#endif /* VM_PROCESS_THREAD_H_ */