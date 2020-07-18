// task.c
//
// Created on: Jul 4, 2020
//     Author: Jeff Manzione

#include "vm/process/task.h"

#include "entity/class/classes.h"
#include "struct/alist.h"
#include "vm/process/context.h"
#include "vm/process/processes.h"

void task_init(Task *task) {
  task->state = TASK_NEW;
  alist_init(&task->context_stack, Context, DEFAULT_ARRAY_SZ);
  alist_init(&task->entity_stack, Entity, DEFAULT_ARRAY_SZ);
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
  ctx->parent_task = task;
  return ctx;
}

Context *task_back_context(Task *task) {
  Context *last = (Context *)alist_get(&task->context_stack,
                                       alist_len(&task->context_stack) - 1);
  context_finalize(last);
  alist_remove_last(&task->context_stack);
  return (Context *)alist_get(&task->context_stack,
                              alist_len(&task->context_stack) - 1);
}

inline Entity task_popstack(Task *task) {
  Entity e = *task_peekstack(task);
  task_dropstack(task);
  return e;
}

inline const Entity *task_peekstack(Task *task) {
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