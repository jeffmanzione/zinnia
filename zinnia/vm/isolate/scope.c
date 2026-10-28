// scope.c
//
// Created on: Jun 13, 2020
//     Author: Jeff Manzione

#include "zinnia/vm/isolate/scope.h"

#include "zinnia/entity/class/classes_def.h"
#include "zinnia/entity/function/function.h"
#include "zinnia/entity/module/modules.h"
#include "zinnia/entity/object.h"
#include "zinnia/program/tape.h"
#include "zinnia/vm/intern.h"
#include "zinnia/vm/isolate/isolates.h"

Heap *scope_heap_(Scope *ctx);

void scope_add_reflection_(Scope *scope) {
  scope->_reflection = heap_new(scope_heap_(scope), Class_Scope);
  scope->_reflection->_internal_obj = scope;
}

void scope_init(Scope *ctx, Object *self, Module *module,
                uint32_t instruction_pos) {
  ctx->self = entity_object(self);
  ctx->module = module;
  ctx->tape = module->_tape;
  ctx->ins = instruction_pos;
  ctx->func = NULL;
  ctx->error = NULL;
  ctx->catch_ins = -1;
  ctx->previous_scope = NULL;
  scope_add_reflection_(ctx);
}

void scope_finalize(Scope *ctx) { ASSERT(ctx != NULL); }

const Instruction *scope_ins(Scope *ctx) {
  ASSERT(ctx != NULL);
  return tape_get(ctx->tape, ctx->ins);
}

Heap *scope_heap_(Scope *ctx) {
  return ctx->parent_frame->parent_isolate->heap;
}

Object *scope_self(Scope *ctx) {
  ASSERT(ctx != NULL);
  return ctx->self.obj;
}

Module *scope_module(Scope *ctx) {
  ASSERT(ctx != NULL);
  return ctx->module;
}

Object *wrap_function_in_ref(const Function *f, Object *obj, Heap *heap,
                             Scope *ctx) {
  Object *fn_ref = heap_new(heap, Class_FunctionRef);
  function_ref_init__(fn_ref, obj, f, f->_is_anon ? ctx : NULL, heap);
  return fn_ref;
}

Entity *scope_lookup(Scope *ctx, const char id[], Entity *tmp) {
  ASSERT(ctx != NULL);
  ASSERT(id != NULL);
  if (SELF == id) {
    return &ctx->self;
  }
  Entity *member = object_get(ctx->_reflection, id);
  if (NULL != member) {
    return member;
  }
  Frame *frame = ctx->parent_frame;
  Scope *parent_scope = ctx->previous_scope;
  while (NULL != parent_scope &&
         NULL == (member = object_get(parent_scope->_reflection, id))) {
    parent_scope = parent_scope->previous_scope;
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
                               frame->parent_isolate->heap, ctx));
      return tmp;
    }
    return member;
  }
  const Function *f = class_get_function(ctx->self.obj->_class, id);
  if (NULL != f) {
    Object *f_ref = wrap_function_in_ref(f, ctx->self.obj,
                                         frame->parent_isolate->heap, ctx);

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
          obj->_function_obj, ctx->self.obj, frame->parent_isolate->heap, ctx));
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

void scope_let(Scope *ctx, const char id[], const Entity *e) {
  ASSERT(ctx != NULL);
  ASSERT(id != NULL);
  ASSERT(e != NULL);
  object_set_member(scope_heap_(ctx), ctx->_reflection, id, e);
}

void scope_set(Scope *ctx, const char id[], const Entity *e) {
  ASSERT(ctx != NULL);
  ASSERT(id != NULL);
  ASSERT(e != NULL);
  Entity *member = NULL;
  if (NULL != object_get(ctx->_reflection, id)) {
    object_set_member(scope_heap_(ctx), ctx->_reflection, id, e);
    return;
  }
  Scope *parent_scope = ctx->previous_scope;
  while (NULL != parent_scope &&
         NULL == (member = object_get(parent_scope->_reflection, id))) {
    parent_scope = parent_scope->previous_scope;
  }
  if (NULL != member) {
    object_set_member(scope_heap_(parent_scope), parent_scope->_reflection, id,
                      e);
    return;
  }
  if (NULL != object_get(ctx->self.obj, id)) {
    object_set_member(scope_heap_(ctx), ctx->self.obj, id, e);
    return;
  }
  object_set_member(scope_heap_(ctx), ctx->_reflection, id, e);
}

void scope_set_function(Scope *ctx, const Function *func) { ctx->func = func; }