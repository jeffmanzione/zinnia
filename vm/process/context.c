// context.c
//
// Created on: Jun 13, 2020
//     Author: Jeff Manzione

#include "vm/process/context.h"

#include "entity/object.h"
#include "program/tape.h"
#include "vm/intern.h"
#include "vm/process/processes.h"

Heap *_context_heap(Context *ctx);

void context_init(Context *ctx, Object *self, Object *member_obj,
                  Module *module, uint32_t instruction_pos) {
  ctx->self = entity_object(self);
  ctx->member_obj = member_obj;
  ctx->module = module;
  ctx->tape = module->_tape;
  ctx->is_function = false;
  ctx->ins = instruction_pos;
}
void context_finalize(Context *ctx) { ASSERT(NOT_NULL(ctx)); }

inline const Instruction *context_ins(Context *ctx) {
  ASSERT(NOT_NULL(ctx));
  return tape_get(ctx->tape, ctx->ins);
}

inline Heap *_context_heap(Context *ctx) {
  return ctx->parent_task->parent_process->heap;
}

inline Object *context_self(Context *ctx) {
  ASSERT(NOT_NULL(ctx));
  return ctx->self.obj;
}

inline Module *context_module(Context *ctx) {
  ASSERT(NOT_NULL(ctx));
  return ctx->module;
}

Entity *context_lookup(Context *ctx, const char id[]) {
  ASSERT(NOT_NULL(ctx), NOT_NULL(id));
  if (SELF == id) {
    return &ctx->self;
  }
  Entity *member = object_get(ctx->member_obj, id);
  if (NULL != member) {
    return member;
  }
  Task *task = ctx->parent_task;
  int32_t index = ctx->index - 1;
  if (index >= 0) {
    Context *parent_context = task_get_context_for_index(task, index);
    while (NULL == (member = object_get(parent_context->member_obj, id))) {
      if (index == 0) {
        break;
      }
      parent_context = task_get_context_for_index(task, --index);
    }
    if (NULL != member) {
      return member;
    }
  }
  member = object_get(ctx->self.obj, id);
  if (NULL != member) {
    return member;
  }
  member = object_get(ctx->module->_reflection, id);
  if (NULL != member) {
    return member;
  }

  Object *obj = module_lookup(ctx->module, id);
  if (NULL != obj) {
    return object_set_member_obj(_context_heap(ctx), ctx->module->_reflection,
                                 id, obj);
  }
  return NULL;
}

void context_let(Context *ctx, const char id[], const Entity *e) {
  ASSERT(NOT_NULL(ctx), NOT_NULL(id), NOT_NULL(e));
  object_set_member(_context_heap(ctx), ctx->member_obj, id, e);
}

void context_set(Context *ctx, const char id[], const Entity *e) {
  ASSERT(NOT_NULL(ctx), NOT_NULL(id), NOT_NULL(e));
  Entity *member;
  if (NULL != object_get(ctx->member_obj, id)) {
    object_set_member(_context_heap(ctx), ctx->member_obj, id, e);
    return;
  }
  Task *task = ctx->parent_task;
  int32_t index = ctx->index - 1;
  if (index >= 0) {
    Context *parent_context = task_get_context_for_index(task, index);
    while (NULL == (member = object_get(parent_context->member_obj, id))) {
      if (index == 0) {
        break;
      }
      parent_context = task_get_context_for_index(task, --index);
    }
    if (NULL != member) {
      object_set_member(_context_heap(parent_context),
                        parent_context->member_obj, id, e);
      return;
    }
  }
  if (NULL != object_get(ctx->self.obj, id)) {
    object_set_member(_context_heap(ctx), ctx->self.obj, id, e);
    return;
  }
  object_set_member(_context_heap(ctx), ctx->member_obj, id, e);
}