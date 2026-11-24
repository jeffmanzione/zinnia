// frame.c
//
// Created on: Jul 4, 2020
//     Author: Jeff Manzione

#include "zinnia/vm/isolate/frame.h"

#include "zinnia/heap/heap.h"
#include "zinnia/util/error.h"
#include "zinnia/value/class/classes_def.h"
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
  SlotStack_init(&frame->slots);
  frame->pending_args = 0;
  frame->parent_frame = NULL;
  FrameSet_init(&frame->dependent_frames, hash_frame, compare_frames);
  frame->child_frame_has_error = false;
  frame->current = NULL;
  // Frames are reused, and a frame that never runs code, such as one that
  // holds a result for a future, must not keep its predecessor's.
  frame->acc = NONE_VALUE;
  frame->_reflection = NULL;
  frame->is_finalized = false;
  frame->actor_future = NULL;
  frame->future = NULL;
  frame->initializing_module = NULL;
}

void frame_finalize(Frame *frame) {
  if (frame->is_finalized) {
    return;
  }
  FrameSet_finalize(&frame->dependent_frames);
  ValueStack_finalize(&frame->value_stack);
  SlotStack_finalize(&frame->slots);
  // Frees the frame's scopes that nothing else can refer to.
  Scope *scope = frame->current;
  while (NULL != scope && scope->parent_frame == frame) {
    Scope *next = scope_dynamic_parent(scope);
    scope_release(scope);
    scope = next;
  }
  frame->current = NULL;
  if (NULL != frame->parent_isolate) {
    heap_dec_edge(frame->parent_isolate->heap,
                  frame->parent_isolate->_reflection, frame->_reflection);
  }
  frame->is_finalized = true;
}

Scope *frame_create_scope(Frame *frame, Object *self, Module *module,
                          uint32_t instruction_pos) {
  Scope *ctx = scope_alloc(frame->parent_isolate);
  ctx->parent_frame = frame;
  ctx->parent_isolate = frame->parent_isolate;
  scope_init(ctx, self, module, instruction_pos);
  // A block's scope uses its function's slots.
  ctx->slot_base = (NULL == frame->current) ? SlotStack_size(&frame->slots)
                                            : frame->current->slot_base;
  scope_set_previous(ctx, frame->current);
  frame->current = ctx;
  return ctx;
}

Scope *frame_back_scope(Frame *frame) {
  Scope *block = frame->current;
  uint32_t ins = block->ins;
  frame->current = block->previous_scope;
  scope_release(block);
  // This was the last context.
  if (NULL == frame->current) {
    return NULL;
  }
  frame->current->ins = ins;
  return frame->current;
}
