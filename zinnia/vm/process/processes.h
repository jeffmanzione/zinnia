// process.h
//
// Created on: Jul 7, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_PROCESS_PROCESSES_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_PROCESS_PROCESSES_H_

#include "c-data-structures/arraylike.h"
#include "c-data-structures/setlike.h"
#include "rzalloc/rzalloc.h"
#include "zinnia/entity/module/module.h"
#include "zinnia/entity/object.h"
#include "zinnia/heap/heap.h"
#include "zinnia/program/tape.h"
#include "zinnia/util/sync/critical_section.h"
#include "zinnia/util/sync/mutex.h"
#include "zinnia/util/sync/thread.h"
#include "zinnia/util/sync/threadpool.h"

typedef struct _VM VM;
typedef struct __Context Context;
typedef struct __Task Task;
typedef struct __Process Process;
typedef struct Future_ Future;

uint32_t hash_task(const Task *tsk, uint32_t size);
int32_t compare_tasks(const Task *tsk1, uint32_t size1, const Task *tsk2,
                      uint32_t size2);

DEFINE_ARRAYLIKE(EntityStack, Entity);
DEFINE_SETLIKE(TaskSet, Task *);
DEFINE_ARRAYLIKE(TaskArray, Task *);

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
  EntityStack entity_stack;

  Task *parent_task;

  TaskSet dependent_tasks;

  bool child_task_has_error;
  bool is_finalized;

  Object *_reflection;

  Future *remote_future;
};

struct __Process {
  VM *vm;
  Heap *heap;

  RzallocArena task_arena;
  RzallocArena context_arena;
  Mutex task_create_lock;
  Mutex task_queue_lock;

  CriticalSection task_waiting_cs;
  Condition *task_wait_cond;

  Mutex task_complete_lock;

  Task *current_task;
  TaskArray queued_tasks;
  TaskSet waiting_tasks;
  TaskSet completed_tasks;
  TaskSet background_tasks;

  Mutex heap_access_lock;

  Object *_reflection;
  ThreadHandle thread;  // Null if main thread.

  VoidPtrArray waiting_background_work;

  Future *future;
  bool is_remote;
  Task *remote_non_daemon_task;
};

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_PROCESS_PROCESSES_H_ */