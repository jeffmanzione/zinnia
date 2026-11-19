// scope.h
//
// Created on: Jun 13, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_ISOLATE_SCOPE_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_ISOLATE_SCOPE_H_

#include "zinnia/program/bytecode.h"
#include "zinnia/program/instruction.h"
#include "zinnia/util/error.h"
#include "zinnia/value/module/module.h"
#include "zinnia/value/object.h"
#include "zinnia/vm/isolate/isolates.h"

void scope_init(Scope *ctx, Object *self, Module *module,
                uint32_t instruction_pos);
void scope_finalize(Scope *ctx);
Object *scope_self(Scope *ctx);
Module *scope_module(Scope *ctx);
static inline const Instruction *scope_ins(Scope *ctx) {
  return bytecode_get(ctx->bytecode, ctx->ins);
}
// The scope that runs after [ctx], in the call stack: its caller, if it was
// called in its caller's frame, or else the scope it's nested in.
Scope *scope_dynamic_parent(const Scope *ctx);
// Sets the scope [ctx] is nested in, which it keeps alive.
void scope_set_previous(Scope *ctx, Scope *previous);
// Frees [ctx], popped from its frame, unless it has an object on the heap, in
// which case the collector frees it once nothing refers to it.
void scope_release(Scope *ctx);
// Keeps [ctx] alive as long as [fn_ref], an anonymous function defined in it.
void scope_capture(Scope *ctx, Object *fn_ref, Heap *heap);
void scope_set_function(Scope *ctx, const Function *func);
Value *scope_lookup(Scope *ctx, const char id[], Value *tmp);
void scope_let(Scope *ctx, const char id[], const Value *e);
// Assigns [e] to the variable [id], wherever it is defined. Returns false
// without assigning if that variable is const.
bool scope_set(Scope *ctx, const char id[], const Value *e);
// Defines [id] in this scope as a const with the value [e]. Returns false
// without assigning if [id] is already a const in this scope.
bool scope_let_const(Scope *ctx, const char id[], const Value *e);
// Returns true if [id] resolves to a const variable from this scope.
bool scope_is_const(Scope *ctx, const char id[]);

// Makes room for [num_slots] unset slots for [ctx]'s function (SLOTS).
void scope_add_slots(Scope *ctx, uint16_t num_slots);
// The same as the functions above for the variable [ins]'s operand names,
// using its slot when it has one.
//
// A variable in a slot is handled inline, since most instructions use one.
void scope_let_ins(Scope *ctx, const Instruction *ins, const Value *e);
bool scope_let_const_ins(Scope *ctx, const Instruction *ins, const Value *e);
// scope_set_ins() for what isn't an assignment to a set, non-const slot.
bool scope_set_ins_slow(Scope *ctx, const Instruction *ins, const Value *e);

// The slot [ins]'s operand names, in [ctx]'s function's slots. [ins] must
// have one.
static inline Slot *scope_slot_ins(Scope *ctx, const Instruction *ins) {
  SlotStack *slots = &ctx->parent_frame->slots;
  const uint32_t index = ctx->slot_base + ins->slot - 1;
  ASSERT(index < slots->size);
  return &slots->table[index];
}

static inline Value *scope_lookup_ins(Scope *ctx, const Instruction *ins,
                                      Value *tmp) {
  if (ins->slot > 0) {
    Slot *slot = scope_slot_ins(ctx, ins);
    if (slot->is_set) {
      return &slot->value;
    }
  }
  return scope_lookup(ctx, ins->id, tmp);
}

static inline bool scope_set_ins(Scope *ctx, const Instruction *ins,
                                 const Value *e) {
  if (ins->slot > 0) {
    Slot *slot = scope_slot_ins(ctx, ins);
    if (slot->is_set && !slot->is_const) {
      slot->value = *e;
      return true;
    }
  }
  return scope_set_ins_slow(ctx, ins, e);
}

static inline bool scope_is_const_ins(Scope *ctx, const Instruction *ins) {
  if (ins->slot > 0) {
    Slot *slot = scope_slot_ins(ctx, ins);
    if (slot->is_set) {
      return slot->is_const;
    }
  }
  return scope_is_const(ctx, ins->id);
}

Scope *frame_get_scope_for_index(Frame *frame, uint32_t index);

Object *wrap_function_in_ref(const Function *f, Object *obj, Heap *heap,
                             Scope *ctx);
#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_ISOLATE_SCOPE_H_ */