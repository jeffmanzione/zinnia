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

  Object *member_obj;

  bool is_function;
  Object *self;
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

struct __Task {
  TaskState state;
  Process *parent_process;

  Entity resval;
  AList entity_stack;
  AList context_stack;
};

struct __Process {
  VM *vm;
  Heap *heap;

  Task *current_task;
  AList task_queue;
  AList waiting_tasks;
  AList completed_tasks;
};

#endif /* VM_PROCESS_PROCESSES_H_ */