// process.h
//
// Created on: Jul 7, 2020
//     Author: Jeff Manzione

#ifndef VM_PROCESS_PROCESSES_H_
#define VM_PROCESS_PROCESSES_H_

#include "alloc/arena/arena.h"
#include "entity/module/module.h"
#include "entity/object.h"
#include "heap/heap.h"
#include "program/tape.h"
#include "struct/alist.h"
#include "struct/set.h"
#include "util/sync/critical_section.h"
#include "util/sync/mutex.h"
#include "util/sync/thread.h"
#include "util/sync/threadpool.h"

typedef struct _VM VM;
typedef struct __Context Context;
typedef struct __Task Task;
typedef struct __Process Process;

struct __Context {
  Task *parent_task;
  Context *previous_context;

  Object *_reflection;

  Entity self;
  Module *module;
  const Tape *tape;
  const Function *func;
  uint32_t ins;

  Object *error;
  int32_t catch_ins;
};

typedef enum {
  TASK_NEW,
  TASK_RUNNING,
  TASK_WAITING,
  TASK_COMPLETE,
  TASK_ERROR,
} TaskState;

typedef enum {
  NOT_WAITING,
  WAITING_TO_START,
  WAITING_ON_FN_CALL,
  WAITING_ON_FUTURE,
} WaitReason;

struct __Task {
  volatile TaskState state;
  WaitReason wait_reason;

  Process *parent_process;
  Context *current;

  Entity resval;
  AList entity_stack;

  Task *parent_task;

  Set dependent_tasks;

  bool child_task_has_error;
  bool is_finalized;

  Object *_reflection;
};

struct __Process {
  VM *vm;
  Heap *heap;

  __Arena task_arena;
  __Arena context_arena;
  Mutex task_create_lock;
  Mutex task_queue_lock;

  CriticalSection task_waiting_cs;
  Condition *task_wait_cond;

  Mutex task_complete_lock;

  Task *current_task;
  Q queued_tasks;
  Set waiting_tasks;
  Set completed_tasks;
  Set background_tasks;

  Mutex heap_access_lock;

  Object *_reflection;
  ThreadHandle thread; // Null if main thread.

  Q waiting_background_work;
};

#endif /* VM_PROCESS_PROCESSES_H_ */