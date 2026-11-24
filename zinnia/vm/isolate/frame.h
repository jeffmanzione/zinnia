// frame.h
//
// Created on: Jul 3, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_ISOLATE_FRAME_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_ISOLATE_FRAME_H_

#include "zinnia/util/error.h"
#include "zinnia/value/value.h"
#include "zinnia/vm/isolate/isolates.h"
#include "zinnia/vm/isolate/scope.h"

const char *frame_state_str(FrameState state);
const char *wait_reason_str(WaitReason reason);

void frame_init(Frame *frame);
void frame_finalize(Frame *frame);
Scope *frame_create_scope(Frame *frame, Object *self, Module *module,
                          uint32_t instruction_pos);
// Starts a call of a function or method in [frame], in a scope of its own,
// which is returned. The scope that was current runs again once
// frame_pop_call_scope() ends the call.
static inline Scope *frame_push_call_scope(Frame *frame, Object *self,
                                           Module *module,
                                           uint32_t instruction_pos) {
  Scope *ctx = scope_alloc(frame->parent_isolate);
  ctx->parent_frame = frame;
  ctx->parent_isolate = frame->parent_isolate;
  scope_init(ctx, self, module, instruction_pos);
  ctx->caller_scope = frame->current;
  ctx->caller_stack_size = frame->value_stack.size;
  ctx->slot_base = frame->slots.size;
  frame->current = ctx;
  return ctx;
}
// Ends [call_scope], a call started by frame_push_call_scope(), and returns to
// its caller.
static inline void frame_pop_call_scope(Frame *frame, Scope *call_scope) {
  ASSERT(NULL != call_scope->caller_scope);
  // Drops what the call left on the stack, and its slots.
  if (frame->value_stack.size > call_scope->caller_stack_size) {
    frame->value_stack.size = call_scope->caller_stack_size;
  }
  if (frame->slots.size > call_scope->slot_base) {
    frame->slots.size = call_scope->slot_base;
  }
  // And the scopes of blocks it returned from inside of, and its own.
  Scope *scope = frame->current;
  frame->current = call_scope->caller_scope;
  while (NULL != scope) {
    Scope *next = (scope == call_scope) ? NULL : scope->previous_scope;
    scope_release(scope);
    scope = next;
  }
}
Scope *frame_back_scope(Frame *frame);

// The value stack and accumulator are used by nearly every instruction, so
// these are inline.
static inline const Value *frame_peekstack(Frame *frame) {
  ASSERT(frame->value_stack.size > 0);
  return &frame->value_stack.table[frame->value_stack.size - 1];
}

static inline const Value *frame_peekstack_n(Frame *frame, int n) {
  ASSERT(frame->value_stack.size > n);
  return &frame->value_stack.table[frame->value_stack.size - 1 - n];
}

static inline void frame_dropstack(Frame *frame) {
  ASSERT(frame->value_stack.size > 0);
  frame->value_stack.size--;
}

static inline Value frame_popstack(Frame *frame) {
  ASSERT(frame->value_stack.size > 0);
  return frame->value_stack.table[--frame->value_stack.size];
}

static inline Value *frame_pushstack(Frame *frame) {
  ValueStack *stack = &frame->value_stack;
  if (stack->size < stack->capacity) {
    return &stack->table[stack->size++];
  }
  return ValueStack_push_back_ref(stack);
}

static inline const Value *frame_get_acc(Frame *frame) { return &frame->acc; }

static inline Value *frame_mutable_acc(Frame *frame) { return &frame->acc; }

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_ISOLATE_FRAME_H_ */