// task.c
//
// Created on: Jul 4, 2020
//     Author: Jeff Manzione

#include "vm/process/task.h"

#include "debug/debug.h"
#include "entity/class/classes.h"
#include "heap/heap.h"
#include "struct/alist.h"
#include "vm/process/context.h"
#include "vm/process/processes.h"

static const char *_task_states[] = {
    "TASK_NEW", "TASK_RUNNING", "TASK_WAITING", "TASK_COMPLETE", "TASK_ERROR",
};

const char *task_state_str(TaskState state) { return _task_states[state]; }

static const char *_wait_reasons[] = {
    "NOT_WAITING",
    "WAITING_TO_START",
    "WAITING_ON_FN_CALL",
    "WAITING_ON_FUTURE",
};

const char *wait_reason_str(WaitReason reason) { return _wait_reasons[reason]; }

void task_init(Task *task) {
  task->state = TASK_NEW;
  task->state = WAITING_TO_START;
  alist_init(&task->entity_stack, Entity, DEFAULT_ARRAY_SZ);
  task->dependent_task = NULL;
  task->child_task_has_error = false;
  task->current = NULL;
  task->_reflection = NULL;
}

void task_finalize(Task *task) {
  alist_finalize(&task->entity_stack);
  if (NULL != task->parent_process) {
    heap_dec_edge(task->parent_process->heap, task->parent_process->_reflection,
                  task->_reflection);
  }
}

Context *task_create_context(Task *task, Object *self, Module *module,
                             uint32_t instruction_pos) {
  Context *ctx = (Context *)__arena_alloc(&task->parent_process->context_arena);
  Object *members_obj = heap_new(task->parent_process->heap, Class_Object);
  context_init(ctx, self, members_obj, module, instruction_pos);
  ctx->parent_task = task;
  ctx->previous_context = task->current;
  task->current = ctx;
  return ctx;
}

Context *task_back_context(Task *task) {
  uint32_t ins = task->current->ins;
  context_finalize(task->current);
  task->current = task->current->previous_context;
  // This was the last context.
  if (NULL == task->current) {
    return NULL;
  }
  task->current->ins = ins;
  return task->current;
}

inline Entity task_popstack(Task *task) {
  ASSERT(alist_len(&task->entity_stack) > 0);
  Entity e = *task_peekstack(task);
  task_dropstack(task);
  return e;
}

inline const Entity *task_peekstack(Task *task) {
  ASSERT(alist_len(&task->entity_stack) > 0);
  return (Entity *)alist_get(&task->entity_stack,
                             alist_len(&task->entity_stack) - 1);
}

inline void task_dropstack(Task *task) {
  alist_remove_last(&task->entity_stack);
}

inline Entity *task_pushstack(Task *task) {
  return (Entity *)alist_add(&task->entity_stack);
}

inline const Entity *task_get_resval(Task *task) { return &task->resval; }

inline Entity *task_mutable_resval(Task *task) { return &task->resval; }
