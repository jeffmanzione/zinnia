// task.c
//
// Created on: Jul 4, 2020
//     Author: Jeff Manzione

#include "vm/process/task.h"

#include "debug/debug.h"
#include "entity/class/classes.h"
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
  alist_init(&task->context_stack, Context, DEFAULT_ARRAY_SZ);
  alist_init(&task->entity_stack, Entity, DEFAULT_ARRAY_SZ);
  task->dependent_task = NULL;
}

void task_finalize(Task *task) {
  alist_finalize(&task->context_stack);
  alist_finalize(&task->entity_stack);
}

Context *task_create_context(Task *task, Object *self, Module *module,
                             uint32_t instruction_pos) {
  Context *ctx = (Context *)alist_add(&task->context_stack);
  Object *members_obj = heap_new(task->parent_process->heap, Class_Object);
  context_init(ctx, self, members_obj, module, instruction_pos);
  ctx->index = alist_len(&task->context_stack) - 1;
  ctx->parent_task = task;
  return ctx;
}

Context *task_back_context(Task *task) {
  Context *last = (Context *)alist_get(&task->context_stack,
                                       alist_len(&task->context_stack) - 1);
  uint32_t ins = last->ins;
  context_finalize(last);
  alist_remove_last(&task->context_stack);
  // This was the last context.
  if (0 == alist_len(&task->context_stack)) {
    return NULL;
  }
  Context *cur = (Context *)alist_get(&task->context_stack,
                                      alist_len(&task->context_stack) - 1);
  cur->ins = ins;
  return cur;
}

inline Entity task_popstack(Task *task) {
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

Context *task_get_context_for_index(Task *task, uint32_t index) {
  ASSERT(NOT_NULL(task), index >= 0, index < alist_len(&task->context_stack));
  return alist_get(&task->context_stack, index);
}