// frame.c
//
// Created on: Jul 4, 2020
//     Author: Jeff Manzione

#include "zinnia/vm/isolate/frame.h"

#include "zinnia/entity/class/classes_def.h"
#include "zinnia/heap/heap.h"
#include "zinnia/util/error.h"
#include "zinnia/vm/isolate/isolates.h"
#include "zinnia/vm/isolate/scope.h"

static const char *_frame_states[] = {
    "TASK_NEW", "TASK_RUNNING", "TASK_WAITING", "TASK_COMPLETE", "TASK_ERROR",
};

const char *frame_state_str(FrameState state) { return _frame_states[state]; }

static const char *_wait_reasons[] = {
    "NOT_WAITING",
    "WAITING_TO_START",
    "WAITING_ON_FN_CALL",
    "WAITING_ON_FUTURE",
    "WAITING_ON_MODULE",
};

const char *wait_reason_str(WaitReason reason) { return _wait_reasons[reason]; }

void frame_init(Frame *frame) {
  frame->state = FRAME_NEW;
  frame->wait_reason = WAITING_TO_START;
  ValueStack_init(&frame->value_stack);
  frame->parent_frame = NULL;
  FrameSet_init(&frame->dependent_frames, hash_frame, compare_frames);
  frame->child_frame_has_error = false;
  frame->current = NULL;
  frame->_reflection = NULL;
  frame->is_finalized = false;
  frame->actor_future = NULL;
  frame->initializing_module = NULL;
}

void frame_finalize(Frame *frame) {
  if (frame->is_finalized) {
    return;
  }
  FrameSet_finalize(&frame->dependent_frames);
  ValueStack_finalize(&frame->value_stack);
  if (NULL != frame->parent_isolate) {
    heap_dec_edge(frame->parent_isolate->heap,
                  frame->parent_isolate->_reflection, frame->_reflection);
  }
  frame->is_finalized = true;
}

Scope *frame_create_scope(Frame *frame, Object *self, Module *module,
                          uint32_t instruction_pos) {
  Scope *ctx = (Scope *)arena_malloc(&frame->parent_isolate->scope_arena);
  ctx->parent_frame = frame;
  scope_init(ctx, self, module, instruction_pos);
  ctx->previous_scope = frame->current;
  frame->current = ctx;
  return ctx;
}

Scope *frame_back_scope(Frame *frame) {
  uint32_t ins = frame->current->ins;
  frame->current = frame->current->previous_scope;
  // This was the last context.
  if (NULL == frame->current) {
    return NULL;
  }
  frame->current->ins = ins;
  return frame->current;
}

Value frame_popstack(Frame *frame) {
  ASSERT(ValueStack_size(&frame->value_stack) > 0);
  Value e = *frame_peekstack(frame);
  frame_dropstack(frame);
  return e;
}

const Value *frame_peekstack(Frame *frame) {
  ASSERT(ValueStack_size(&frame->value_stack) > 0);
  return ValueStack_last_ref_unchecked(&frame->value_stack);
}

const Value *frame_peekstack_n(Frame *frame, int n) {
  ASSERT(ValueStack_size(&frame->value_stack) > n);
  return ValueStack_get_ref_unchecked(
      &frame->value_stack, ValueStack_size(&frame->value_stack) - 1 - n);
}

void frame_dropstack(Frame *frame) {
  ValueStack_pop_back_unchecked(&frame->value_stack);
}

Value *frame_pushstack(Frame *frame) {
  return ValueStack_push_back_ref(&frame->value_stack);
}

const Value *frame_get_acc(Frame *frame) { return &frame->acc; }

Value *frame_mutable_acc(Frame *frame) { return &frame->acc; }
