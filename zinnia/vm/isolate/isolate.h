// isolate.h
//
// Created on: Jul 3, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_ISOLATE_ISOLATE_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_ISOLATE_ISOLATE_H_

#include "zinnia/vm/isolate/isolates.h"

void isolate_init(Isolate *isolate, HeapConf *config);
void isolate_finalize(Isolate *isolate);
Frame *isolate_create_unqueued_frame(Isolate *isolate);
Frame *isolate_create_frame(Isolate *isolate);

Frame *isolate_pop_frame(Isolate *isolate);
void isolate_push_frame(Isolate *isolate, Frame *frame);
void isolate_enqueue_frame(Isolate *isolate, Frame *frame);
size_t isolate_queue_size(Isolate *isolate);

void isolate_insert_waiting_frame(Isolate *isolate, Frame *frame);
void isolate_remove_waiting_frame(Isolate *isolate, Frame *frame);
void isolate_mark_frame_complete(Isolate *isolate, Frame *frame);

void isolate_add_blocking_frame(Isolate *isolate, Frame *frame,
                                ThreadPool *blocking_pool, VoidFnPtr fn,
                                VoidFnPtr callback, VoidPtr fn_args);
void isolate_remove_blocking_frame(Isolate *isolate, Frame *frame);

void isolate_delete_frame(Isolate *isolate, Frame *frame);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_ISOLATE_ISOLATE_H_ */