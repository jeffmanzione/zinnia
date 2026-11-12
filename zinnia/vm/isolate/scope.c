// scope.c
//
// Created on: Jun 13, 2020
//     Author: Jeff Manzione

#include "zinnia/vm/isolate/scope.h"

#include "zinnia/alloc/alloc.h"
#include "zinnia/program/bytecode.h"
#include "zinnia/value/class/classes_def.h"
#include "zinnia/value/function/function.h"
#include "zinnia/value/module/modules.h"
#include "zinnia/value/object.h"
#include "zinnia/vm/intern.h"
#include "zinnia/vm/isolate/isolates.h"

Heap *scope_heap_(Scope *ctx);

void scope_add_reflection_(Scope *scope) {
  scope->_reflection = heap_new(scope_heap_(scope), Class_Scope);
  scope->_reflection->_internal_obj = scope;
}

void scope_init(Scope *ctx, Object *self, Module *module,
                uint32_t instruction_pos) {
  ctx->self = value_object(self);
  ctx->module = module;
  ctx->bytecode = module->_bytecode;
  ctx->ins = instruction_pos;
  ctx->func = NULL;
  ctx->error = NULL;
  ctx->catch_ins = -1;
  ctx->previous_scope = NULL;
  ctx->const_ids = NULL;
  ctx->num_const_ids = 0;
  ctx->const_ids_capacity = 0;
  scope_add_reflection_(ctx);
}

void scope_finalize(Scope *ctx) {
  ASSERT(ctx != NULL);
  if (NULL != ctx->const_ids) {
    RELEASE(ctx->const_ids);
    ctx->const_ids = NULL;
  }
}

static bool scope_is_const_(const Scope *ctx, const char id[]) {
  for (int i = 0; i < ctx->num_const_ids; ++i) {
    if (ctx->const_ids[i] == id) {
      return true;
    }
  }
  return false;
}

const Instruction *scope_ins(Scope *ctx) {
  ASSERT(ctx != NULL);
  return bytecode_get(ctx->bytecode, ctx->ins);
}

Heap *scope_heap_(Scope *ctx) { return ctx->parent_isolate->heap; }

void scope_set_previous(Scope *ctx, Scope *previous) {
  ctx->previous_scope = previous;
  // An anonymous function defined in [ctx] can outlive it, and looks up names
  // in the scopes it's nested in, so they're kept as long as it is.
  if (NULL != previous && previous->parent_isolate == ctx->parent_isolate) {
    heap_inc_edge(scope_heap_(ctx), ctx->_reflection, previous->_reflection);
  }
}

void scope_capture(Scope *ctx, Object *fn_ref, Heap *heap) {
  // An anonymous function keeps the scope it was defined in.
  if (NULL != ctx && scope_heap_(ctx) == heap) {
    heap_inc_edge(heap, fn_ref, ctx->_reflection);
  }
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
  if (f->_is_anon) {
    scope_capture(ctx, fn_ref, heap);
  }
  return fn_ref;
}

Value *scope_lookup(Scope *ctx, const char id[], Value *tmp) {
  ASSERT(ctx != NULL);
  ASSERT(id != NULL);
  if (SELF == id) {
    return &ctx->self;
  }
  Value *member = object_get(ctx->_reflection, id);
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
      *tmp = value_object(
          wrap_function_in_ref(member->obj->_function_obj, ctx->self.obj,
                               frame->parent_isolate->heap, ctx));
      return tmp;
    }
    return member;
  }
  const Function *f = class_get_function(ctx->self.obj->_class, id);
  // Generic methods inherited from Object (e.g., Object.hash()) and, in
  // module-level code, methods of the module object itself would otherwise
  // shadow module members and builtins with the same name (e.g., hash()). Only
  // fall back to them when nothing else matches.
  const Function *deferred_module_fn = NULL;
  if (NULL != f && (Class_Object == f->_parent_class ||
                    ctx->self.obj == ctx->module->_reflection)) {
    deferred_module_fn = f;
    f = NULL;
  }
  if (NULL != f) {
    Object *f_ref = wrap_function_in_ref(f, ctx->self.obj,
                                         frame->parent_isolate->heap, ctx);

    // TODO: With this uncommented code, the function ref can be collected when
    // allocated by a different heap from the object. This needs to be resolved.
    //
    // if (f->_is_anon) {
    //   *tmp = value_object(f_ref);
    //   return tmp;
    // } else {
    //   return object_set_member_obj(task->parent_process->heap, ctx->self.obj,
    //                                id, f_ref);
    // }

    *tmp = value_object(f_ref);
    return tmp;
  }
  member = object_get(ctx->module->_reflection, id);
  if (NULL != member) {
    return member;
  }

  Object *obj = module_lookup(ctx->module, id);
  if (NULL != obj) {
    if (Class_Function == obj->_class && obj->_function_obj->_is_anon) {
      *tmp = value_object(wrap_function_in_ref(
          obj->_function_obj, ctx->self.obj, frame->parent_isolate->heap, ctx));
      return tmp;
    }
    *tmp = value_object(obj);
    return tmp;
  }

  member = object_get(Module_builtin->_reflection, id);
  if (NULL != member) {
    return member;
  }

  obj = module_lookup(Module_builtin, id);
  if (NULL != obj) {
    *tmp = value_object(obj);
    return tmp;
  }
  if (NULL != deferred_module_fn) {
    *tmp = value_object(wrap_function_in_ref(deferred_module_fn, ctx->self.obj,
                                             frame->parent_isolate->heap, ctx));
    return tmp;
  }
  return NULL;
}

void scope_let(Scope *ctx, const char id[], const Value *e) {
  ASSERT(ctx != NULL);
  ASSERT(id != NULL);
  ASSERT(e != NULL);
  object_set_member(scope_heap_(ctx), ctx->_reflection, id, e);
}

bool scope_is_const(Scope *ctx, const char id[]) {
  for (Scope *s = ctx; NULL != s; s = s->previous_scope) {
    if (NULL != object_get(s->_reflection, id)) {
      return scope_is_const_(s, id);
    }
  }
  return false;
}

bool scope_let_const(Scope *ctx, const char id[], const Value *e) {
  ASSERT(ctx != NULL);
  ASSERT(id != NULL);
  ASSERT(e != NULL);
  if (scope_is_const_(ctx, id)) {
    return false;
  }
  if (ctx->num_const_ids == ctx->const_ids_capacity) {
    ctx->const_ids_capacity =
        (0 == ctx->const_ids_capacity) ? 4 : ctx->const_ids_capacity * 2;
    ctx->const_ids =
        (NULL == ctx->const_ids)
            ? MNEW_ARR(const char *, ctx->const_ids_capacity)
            : REALLOC(ctx->const_ids, const char *, ctx->const_ids_capacity);
  }
  ctx->const_ids[ctx->num_const_ids++] = id;
  object_set_member(scope_heap_(ctx), ctx->_reflection, id, e);
  return true;
}

bool scope_set(Scope *ctx, const char id[], const Value *e) {
  ASSERT(ctx != NULL);
  ASSERT(id != NULL);
  ASSERT(e != NULL);
  Value *member = NULL;
  if (NULL != object_get(ctx->_reflection, id)) {
    if (scope_is_const_(ctx, id)) {
      return false;
    }
    object_set_member(scope_heap_(ctx), ctx->_reflection, id, e);
    return true;
  }
  Scope *parent_scope = ctx->previous_scope;
  while (NULL != parent_scope &&
         NULL == (member = object_get(parent_scope->_reflection, id))) {
    parent_scope = parent_scope->previous_scope;
  }
  if (NULL != member) {
    if (scope_is_const_(parent_scope, id)) {
      return false;
    }
    object_set_member(scope_heap_(parent_scope), parent_scope->_reflection, id,
                      e);
    return true;
  }
  if (NULL != object_get(ctx->self.obj, id)) {
    object_set_member(scope_heap_(ctx), ctx->self.obj, id, e);
    return true;
  }
  object_set_member(scope_heap_(ctx), ctx->_reflection, id, e);
  return true;
}

void scope_set_function(Scope *ctx, const Function *func) { ctx->func = func; }
