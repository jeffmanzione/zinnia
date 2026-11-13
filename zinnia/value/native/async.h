// async.h
//
// Created on: Nov 11, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_VALUE_NATIVE_ASYNC_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_VALUE_NATIVE_ASYNC_H_

#include "zinnia/value/native/native_hdrs.h"

typedef struct Future_ Future;

Object *future_create(Frame *frame);
// Gives [frame]'s future, if it has one, the frame's result, which it keeps
// once the frame is deleted. Called when the frame completes.
void future_complete_from_frame(Frame *frame);
bool future_is_complete(Future *f);
// Returns whether the future completed with an error.
bool future_is_error(Future *f);
const Value *future_get_value(Heap *heap, Object *obj);
Frame *future_get_frame(Future *f);

void async_add_native(ModuleManager *mm, Module *async);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_VALUE_NATIVE_ASYNC_H_ */