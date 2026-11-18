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

// A scope's object on the heap holds its named variables, and lets the
// collector keep a scope that outlives its call, such as one a lambda
// captured. Most calls keep their variables in slots and never need one, so
// it's made only when first needed. A scope that has one keeps the scopes it's
// nested in, which then have one too.
static Object *scope_reflection_(Scope *ctx) {
  if (NULL != ctx->_reflection) {
    return ctx->_reflection;
  }
  ctx->_reflection = heap_new(scope_heap_(ctx), Class_Scope);
  ctx->_reflection->_internal_obj = ctx;
  Scope *previous = ctx->previous_scope;
  if (NULL != previous && previous->parent_isolate == ctx->parent_isolate) {
    heap_inc_edge(scope_heap_(ctx), ctx->_reflection,
                  scope_reflection_(previous));
  }
  return ctx->_reflection;
}

// The named variable [id] of [ctx], if it has one.
static Value *scope_get_(Scope *ctx, const char id[]) {
  return NULL == ctx->_reflection ? NULL : object_get(ctx->_reflection, id);
}

void scope_release(Scope *ctx) {
  // A scope with an object on the heap is freed when the object is collected.
  if (NULL != ctx->_reflection) {
    return;
  }
  scope_finalize(ctx);
  arena_free(&ctx->parent_isolate->scope_arena, ctx);
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
  ctx->caller_scope = NULL;
  ctx->caller_stack_size = 0;
  ctx->slot_base = 0;
  ctx->is_block = false;
  ctx->const_ids = NULL;
  ctx->num_const_ids = 0;
  ctx->const_ids_capacity = 0;
  ctx->_reflection = NULL;
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
  if (NULL != ctx->_reflection && NULL != previous &&
      previous->parent_isolate == ctx->parent_isolate) {
    heap_inc_edge(scope_heap_(ctx), ctx->_reflection,
                  scope_reflection_(previous));
  }
}

Scope *scope_dynamic_parent(const Scope *ctx) {
  return NULL != ctx->caller_scope ? ctx->caller_scope : ctx->previous_scope;
}

void scope_capture(Scope *ctx, Object *fn_ref, Heap *heap) {
  // An anonymous function keeps the scope it was defined in.
  if (NULL != ctx && scope_heap_(ctx) == heap) {
    heap_inc_edge(heap, fn_ref, scope_reflection_(ctx));
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
  // A method's FunctionRef depends only on its object, so it's reused.
  if (!f->_is_anon) {
    Object *bound = heap_bound_method(heap, obj, f);
    if (NULL != bound) {
      return bound;
    }
  }
  Object *fn_ref = heap_new(heap, Class_FunctionRef);
  function_ref_init__(fn_ref, obj, f, f->_is_anon ? ctx : NULL, heap);
  if (f->_is_anon) {
    scope_capture(ctx, fn_ref, heap);
  } else {
    heap_set_bound_method(heap, obj, f, fn_ref);
  }
  return fn_ref;
}

Value *scope_lookup(Scope *ctx, const char id[], Value *tmp) {
  ASSERT(ctx != NULL);
  ASSERT(id != NULL);
  if (SELF == id) {
    return &ctx->self;
  }
  Value *member = scope_get_(ctx, id);
  if (NULL != member) {
    return member;
  }
  Frame *frame = ctx->parent_frame;
  Scope *parent_scope = ctx->previous_scope;
  while (NULL != parent_scope &&
         NULL == (member = scope_get_(parent_scope, id))) {
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

// The scope new variables of [ctx] go in: its function's, since variables are
// function-wide.
static Scope *function_scope_(Scope *ctx) {
  while (ctx->is_block && NULL != ctx->previous_scope) {
    ctx = ctx->previous_scope;
  }
  return ctx;
}

void scope_let(Scope *ctx, const char id[], const Value *e) {
  ASSERT(ctx != NULL);
  ASSERT(id != NULL);
  ASSERT(e != NULL);
  ctx = function_scope_(ctx);
  object_set_member(scope_heap_(ctx), scope_reflection_(ctx), id, e);
}

bool scope_is_const(Scope *ctx, const char id[]) {
  for (Scope *s = ctx; NULL != s; s = s->previous_scope) {
    if (NULL != scope_get_(s, id)) {
      return scope_is_const_(s, id);
    }
  }
  return false;
}

bool scope_let_const(Scope *ctx, const char id[], const Value *e) {
  ASSERT(ctx != NULL);
  ASSERT(id != NULL);
  ASSERT(e != NULL);
  ctx = function_scope_(ctx);
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
  object_set_member(scope_heap_(ctx), scope_reflection_(ctx), id, e);
  return true;
}

bool scope_set(Scope *ctx, const char id[], const Value *e) {
  ASSERT(ctx != NULL);
  ASSERT(id != NULL);
  ASSERT(e != NULL);
  Value *member = NULL;
  if (NULL != scope_get_(ctx, id)) {
    if (scope_is_const_(ctx, id)) {
      return false;
    }
    object_set_member(scope_heap_(ctx), scope_reflection_(ctx), id, e);
    return true;
  }
  Scope *parent_scope = ctx->previous_scope;
  while (NULL != parent_scope &&
         NULL == (member = scope_get_(parent_scope, id))) {
    parent_scope = parent_scope->previous_scope;
  }
  if (NULL != member) {
    if (scope_is_const_(parent_scope, id)) {
      return false;
    }
    object_set_member(scope_heap_(parent_scope), scope_reflection_(parent_scope), id,
                      e);
    return true;
  }
  if (NULL != object_get(ctx->self.obj, id)) {
    object_set_member(scope_heap_(ctx), ctx->self.obj, id, e);
    return true;
  }
  Scope *function_scope = function_scope_(ctx);
  object_set_member(scope_heap_(function_scope), scope_reflection_(function_scope),
                    id, e);
  return true;
}

// The slot that [ins]'s operand names, in [ctx]'s function's slots.
static Slot *slot_of_(Scope *ctx, const Instruction *ins) {
  SlotStack *slots = &ctx->parent_frame->slots;
  const uint32_t index = ctx->slot_base + ins->slot - 1;
  ASSERT(index < SlotStack_size(slots));
  return SlotStack_mutable_ref_unchecked(slots, index);
}

void scope_add_slots(Scope *ctx, uint16_t num_slots) {
  SlotStack *slots = &ctx->parent_frame->slots;
  ctx->slot_base = SlotStack_size(slots);
  for (int i = 0; i < num_slots; ++i) {
    SlotStack_push_back(slots, (Slot){.value = NONE_VALUE});
  }
}

Value *scope_lookup_ins(Scope *ctx, const Instruction *ins, Value *tmp) {
  if (ins->slot > 0) {
    Slot *slot = slot_of_(ctx, ins);
    if (slot->is_set) {
      return &slot->value;
    }
  }
  return scope_lookup(ctx, ins->id, tmp);
}

void scope_let_ins(Scope *ctx, const Instruction *ins, const Value *e) {
  if (0 == ins->slot) {
    scope_let(ctx, ins->id, e);
    return;
  }
  Slot *slot = slot_of_(ctx, ins);
  slot->value = *e;
  slot->is_set = true;
  slot->is_const = false;
}

bool scope_set_ins(Scope *ctx, const Instruction *ins, const Value *e) {
  if (0 == ins->slot) {
    return scope_set(ctx, ins->id, e);
  }
  Slot *slot = slot_of_(ctx, ins);
  if (slot->is_set) {
    if (slot->is_const) {
      return false;
    }
    slot->value = *e;
    return true;
  }
  // Not created yet in this call: a member of self of that name, such as a
  // field, is assigned instead, as scope_set() does.
  if (NULL != object_get(ctx->self.obj, ins->id)) {
    object_set_member(scope_heap_(ctx), ctx->self.obj, ins->id, e);
    return true;
  }
  slot->value = *e;
  slot->is_set = true;
  return true;
}

bool scope_let_const_ins(Scope *ctx, const Instruction *ins, const Value *e) {
  if (0 == ins->slot) {
    return scope_let_const(ctx, ins->id, e);
  }
  Slot *slot = slot_of_(ctx, ins);
  if (slot->is_set && slot->is_const) {
    return false;
  }
  slot->value = *e;
  slot->is_set = true;
  slot->is_const = true;
  return true;
}

bool scope_is_const_ins(Scope *ctx, const Instruction *ins) {
  if (ins->slot > 0) {
    Slot *slot = slot_of_(ctx, ins);
    if (slot->is_set) {
      return slot->is_const;
    }
  }
  return scope_is_const(ctx, ins->id);
}

void scope_set_function(Scope *ctx, const Function *func) { ctx->func = func; }
