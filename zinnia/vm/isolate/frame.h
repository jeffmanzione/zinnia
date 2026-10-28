// frame.h
//
// Created on: Jul 3, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_ISOLATE_FRAME_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_ISOLATE_FRAME_H_

#include "zinnia/entity/value.h"
#include "zinnia/vm/isolate/isolates.h"

const char *frame_state_str(FrameState state);
const char *wait_reason_str(WaitReason reason);

void frame_init(Frame *frame);
void frame_finalize(Frame *frame);
Scope *frame_create_scope(Frame *frame, Object *self, Module *module,
                          uint32_t instruction_pos);
Scope *frame_back_scope(Frame *frame);

Value frame_popstack(Frame *frame);
const Value *frame_peekstack(Frame *frame);
const Value *frame_peekstack_n(Frame *frame, int n);
void frame_dropstack(Frame *frame);
Value *frame_pushstack(Frame *frame);

const Value *frame_get_acc(Frame *frame);
Value *frame_mutable_acc(Frame *frame);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_ISOLATE_FRAME_H_ */