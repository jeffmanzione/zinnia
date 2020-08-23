// process.h
//
// Created on: Jul 7, 2020
//     Author: Jeff Manzione

#ifndef VM_PROCESS_PROCESSES_H_
#define VM_PROCESS_PROCESSES_H_

#include "entity/module/module.h"
#include "entity/object.h"
#include "heap/heap.h"
#include "program/tape.h"
#include "struct/alist.h"
#include "struct/set.h"

typedef struct _VM VM;
typedef struct __Task Task;
typedef struct __Process Process;

typedef struct {
  Task *parent_task;
  uint32_t index;

  Object *member_obj;

  bool is_function;
  Entity self;
  Module *module;
  const Tape *tape;
  uint32_t ins;
} Context;

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
  TaskState state;
  WaitReason wait_reason;

  Process *parent_process;

  Entity resval;
  AList entity_stack;
  AList context_stack;

  Task *dependent_task;
};

struct __Process {
  VM *vm;
  Heap *heap;

  __Arena task_arena;

  Task *current_task;
  Q queued_tasks;
  Set waiting_tasks;
  Set completed_tasks;
};

#endif /* VM_PROCESS_PROCESSES_H_ */