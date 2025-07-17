// context.c
//
// Created on: Jun 13, 2020
//     Author: Jeff Manzione

#include "vm/process/context.h"

#include "entity/class/classes_def.h"
#include "entity/function/function.h"
#include "entity/module/modules.h"
#include "entity/object.h"
#include "program/tape.h"
#include "vm/intern.h"
#include "vm/process/processes.h"

Heap *_context_heap(Context *ctx);

void _context_add_reflection(Context *context) {
  context->_reflection = heap_new(_context_heap(context), Class_Context);
  context->_reflection->_internal_obj = context;
}

void context_init(Context *ctx, Object *self, Module *module,
                  uint32_t instruction_pos) {
  ctx->self = entity_object(self);
  ctx->module = module;
  ctx->tape = module->_tape;
  ctx->ins = instruction_pos;
  ctx->func = NULL;
  ctx->error = NULL;
  ctx->catch_ins = -1;
  ctx->previous_context = NULL;
  _context_add_reflection(ctx);
}

void context_finalize(Context *ctx) { ASSERT(NOT_NULL(ctx)); }

const Instruction *context_ins(Context *ctx) {
  ASSERT(NOT_NULL(ctx));
  return tape_get(ctx->tape, ctx->ins);
}

Heap *_context_heap(Context *ctx) {
  return ctx->parent_task->parent_process->heap;
}

Object *context_self(Context *ctx) {
  ASSERT(NOT_NULL(ctx));
  return ctx->self.obj;
}

Module *context_module(Context *ctx) {
  ASSERT(NOT_NULL(ctx));
  return ctx->module;
}

Object *wrap_function_in_ref(const Function *f, Object *obj, Heap *heap,
                             Context *ctx) {
  Object *fn_ref = heap_new(heap, Class_FunctionRef);
  __function_ref_init(fn_ref, obj, f, f->_is_anon ? ctx : NULL, heap);
  return fn_ref;
}

Entity *context_lookup(Context *ctx, const char id[], Entity *tmp) {
  ASSERT(NOT_NULL(ctx), NOT_NULL(id));
  if (SELF == id) {
    return &ctx->self;
  }
  Entity *member = object_get(ctx->_reflection, id);
  if (NULL != member) {
    return member;
  }
  Task *task = ctx->parent_task;
  Context *parent_context = ctx->previous_context;
  while (NULL != parent_context &&
         NULL == (member = object_get(parent_context->_reflection, id))) {
    parent_context = parent_context->previous_context;
  }
  if (NULL != member) {
    return member;
  }
  member = object_get(ctx->self.obj, id);
  if (NULL != member) {
    if (OBJECT == member->type && Class_Function == member->obj->_class &&
        member->obj->_function_obj->_is_anon) {
      *tmp = entity_object(
          wrap_function_in_ref(member->obj->_function_obj, ctx->self.obj,
                               task->parent_process->heap, ctx));
      return tmp;
    }
    return member;
  }
  const Function *f = class_get_function(ctx->self.obj->_class, id);
  if (NULL != f) {
    Object *f_ref =
        wrap_function_in_ref(f, ctx->self.obj, task->parent_process->heap, ctx);

    // TODO: With this uncommented code, the function ref can be collected when
    // allocated by a different heap from the object. This needs to be resolved.
    //
    // if (f->_is_anon) {
    //   *tmp = entity_object(f_ref);
    //   return tmp;
    // } else {
    //   return object_set_member_obj(task->parent_process->heap, ctx->self.obj,
    //                                id, f_ref);
    // }

    *tmp = entity_object(f_ref);
    return tmp;
  }
  member = object_get(ctx->module->_reflection, id);
  if (NULL != member) {
    return member;
  }

  Object *obj = module_lookup(ctx->module, id);
  if (NULL != obj) {
    if (Class_Function == obj->_class && obj->_function_obj->_is_anon) {
      *tmp = entity_object(wrap_function_in_ref(
          obj->_function_obj, ctx->self.obj, task->parent_process->heap, ctx));
      return tmp;
    }
    *tmp = entity_object(obj);
    return tmp;
  }

  member = object_get(Module_builtin->_reflection, id);
  if (NULL != member) {
    return member;
  }

  obj = module_lookup(Module_builtin, id);
  if (NULL != obj) {
    *tmp = entity_object(obj);
    return tmp;
  }
  return NULL;
}

void context_let(Context *ctx, const char id[], const Entity *e) {
  ASSERT(NOT_NULL(ctx), NOT_NULL(id), NOT_NULL(e));
  object_set_member(_context_heap(ctx), ctx->_reflection, id, e);
}

void context_set(Context *ctx, const char id[], const Entity *e) {
  ASSERT(NOT_NULL(ctx), NOT_NULL(id), NOT_NULL(e));
  Entity *member = NULL;
  if (NULL != object_get(ctx->_reflection, id)) {
    object_set_member(_context_heap(ctx), ctx->_reflection, id, e);
    return;
  }
  Context *parent_context = ctx->previous_context;
  while (NULL != parent_context &&
         NULL == (member = object_get(parent_context->_reflection, id))) {
    parent_context = parent_context->previous_context;
  }
  if (NULL != member) {
    object_set_member(_context_heap(parent_context),
                      parent_context->_reflection, id, e);
    return;
  }
  if (NULL != object_get(ctx->self.obj, id)) {
    object_set_member(_context_heap(ctx), ctx->self.obj, id, e);
    return;
  }
  object_set_member(_context_heap(ctx), ctx->_reflection, id, e);
}

void context_set_function(Context *ctx, const Function *func) {
  ctx->func = func;
}