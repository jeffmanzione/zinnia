// isolate.c
//
// Created on: Jul 4, 2020
//     Author: Jeff Manzione

#include "zinnia/vm/isolate/isolate.h"

#include "rzalloc/rzalloc.h"
#include "zinnia/entity/class/classes_def.h"
#include "zinnia/util/sync/mutex.h"
#include "zinnia/vm/isolate/frame.h"
#include "zinnia/vm/isolate/isolates.h"

void isolate_init(Isolate *isolate, HeapConf *conf) {
  isolate->heap = heap_create(conf);
  arena_init(&isolate->frame_arena, sizeof(Frame));
  arena_init(&isolate->scope_arena, sizeof(Scope));
  isolate->frame_create_lock = mutex_create();
  isolate->frame_queue_lock = mutex_create();
  isolate->heap_access_lock = mutex_create();
  isolate->frame_waiting_cs = critical_section_create();
  isolate->frame_wait_cond =
      critical_section_create_condition(isolate->frame_waiting_cs);
  isolate->frame_complete_lock = mutex_create();
  FrameArray_init(&isolate->queued_frames);
  FrameSet_init(&isolate->waiting_frames, hash_frame, compare_frames);
  FrameSet_init(&isolate->completed_frames, hash_frame, compare_frames);
  FrameSet_init(&isolate->blocking_frames, hash_frame, compare_frames);
  isolate->_reflection = NULL;
  VoidPtrArray_init(&isolate->waiting_blocking_work);
  isolate->future = NULL;
  isolate->keep_alive = false;
  isolate->keep_alive_frame = NULL;
}

void isolate_finalize(Isolate *isolate) {
  FrameArrayIterator q_iter;
  FrameArray_iterator(&q_iter, &isolate->queued_frames);
  for (; FrameArray_has_next(&q_iter); FrameArray_next(&q_iter)) {
    frame_finalize(*FrameArray_mutable_value(&q_iter));
  }
  FrameArray_finalize(&isolate->queued_frames);

  FrameSetIterator m_iter;
  FrameSet_iterator(&m_iter, &isolate->waiting_frames);
  for (; FrameSet_has_next(&m_iter); FrameSet_next(&m_iter)) {
    frame_finalize(*FrameSet_mutable_value(&m_iter));
  }
  FrameSet_finalize(&isolate->waiting_frames);

  FrameSet_iterator(&m_iter, &isolate->completed_frames);
  for (; FrameSet_has_next(&m_iter); FrameSet_next(&m_iter)) {
    frame_finalize(*FrameSet_mutable_value(&m_iter));
  }
  FrameSet_finalize(&isolate->completed_frames);

  // Is this necessary?
  FrameSet_iterator(&m_iter, &isolate->blocking_frames);
  for (; FrameSet_has_next(&m_iter); FrameSet_next(&m_iter)) {
    frame_finalize(*FrameSet_mutable_value(&m_iter));
  }
  FrameSet_finalize(&isolate->blocking_frames);

  arena_clear(&isolate->frame_arena);
  arena_clear(&isolate->scope_arena);
  heap_delete(isolate->heap);
  mutex_close(isolate->frame_create_lock);
  mutex_close(isolate->frame_queue_lock);
  mutex_close(isolate->heap_access_lock);
  condition_delete(isolate->frame_wait_cond);
  critical_section_delete(isolate->frame_waiting_cs);
  mutex_close(isolate->frame_complete_lock);
  VoidPtrArray_finalize(&isolate->waiting_blocking_work);
}

void frame_add_reflection_(Isolate *isolate, Frame *frame) {
  frame->_reflection = heap_new(isolate->heap, Class_Frame);
  frame->_reflection->_internal_obj = frame;
}

Frame *isolate_create_unqueued_frame(Isolate *isolate) {
  Frame *frame;
  SYNCHRONIZED(isolate->frame_create_lock, {
    frame = (Frame *)arena_malloc(&isolate->frame_arena);
    frame_init(frame);
    frame->parent_isolate = isolate;
    frame_add_reflection_(isolate, frame);
    heap_inc_edge(isolate->heap, isolate->_reflection, frame->_reflection);
  });
  return frame;
}

Frame *isolate_create_frame(Isolate *isolate) {
  Frame *frame = isolate_create_unqueued_frame(isolate);
  isolate_enqueue_frame(isolate, frame);
  return frame;
}

Frame *isolate_pop_frame(Isolate *isolate) {
  Frame *frame;
  SYNCHRONIZED(isolate->frame_queue_lock, {
    if (FrameArray_is_empty(&isolate->queued_frames)) {
      frame = NULL;
    } else {
      frame = FrameArray_pop_front_unchecked(&isolate->queued_frames);
    }
  });

  // Task can be complete if it was prioritized to the front of the queue.
  // This happens to a parent task when a child has an error.
  if (NULL != frame && FRAME_COMPLETE == frame->state) {
    return isolate_pop_frame(isolate);
  }
  return frame;
}

void isolate_enqueue_frame(Isolate *isolate, Frame *frame) {
  SYNCHRONIZED(isolate->frame_queue_lock,
               { *FrameArray_push_back_ref(&isolate->queued_frames) = frame; });
}

void isolate_push_frame(Isolate *isolate, Frame *frame) {
  SYNCHRONIZED(isolate->frame_queue_lock,
               { FrameArray_push_back(&isolate->queued_frames, frame); });
}

size_t isolate_queue_size(Isolate *isolate) {
  size_t size;
  SYNCHRONIZED(isolate->frame_queue_lock,
               { size = FrameArray_size(&isolate->queued_frames); });
  return size;
}

void isolate_insert_waiting_frame(Isolate *isolate, Frame *frame) {
  frame->state = FRAME_WAITING;
  CRITICAL(isolate->frame_waiting_cs, {
    FrameSet_insert(&isolate->waiting_frames, frame, sizeof(Frame *));
  });
}

void isolate_remove_waiting_frame(Isolate *isolate, Frame *frame) {
  // Wakes the isolate, which may be waiting for its waiting frames to change.
  CRITICAL(isolate->frame_waiting_cs, {
    FrameSet_remove(&isolate->waiting_frames, frame, sizeof(Frame *));
    condition_broadcast(isolate->frame_wait_cond);
  });
}

void isolate_enqueue_frame_and_wake(Isolate *isolate, Frame *frame) {
  // Holds the wait lock so the isolate cannot miss the wakeup between checking
  // its queue and waiting.
  CRITICAL(isolate->frame_waiting_cs, {
    isolate_enqueue_frame(isolate, frame);
    condition_broadcast(isolate->frame_wait_cond);
  });
}

bool isolate_wait_on_frame(Frame *frame, Frame *waiting_frame) {
  bool is_waiting = false;
  // Holds the lock that completing the frame wakes its dependents under, so
  // the frame cannot complete between the check and the registration.
  CRITICAL(frame->parent_isolate->frame_waiting_cs, {
    if (FRAME_COMPLETE != frame->state && FRAME_ERROR != frame->state) {
      FrameSet_insert(&frame->dependent_frames, waiting_frame,
                      sizeof(Frame *));
      isolate_insert_waiting_frame(waiting_frame->parent_isolate,
                                   waiting_frame);
      is_waiting = true;
    }
  });
  return is_waiting;
}

void isolate_mark_frame_complete(Isolate *isolate, Frame *frame) {
  SYNCHRONIZED(isolate->frame_complete_lock, {
    FrameSet_insert(&isolate->completed_frames, frame, sizeof(Frame *));
  });
}

void isolate_add_blocking_frame(Isolate *isolate, Frame *frame,
                                ThreadPool *blocking_pool, VoidFnPtr fn,
                                VoidFnPtr callback, VoidPtr fn_args) {
  SYNCHRONIZED(isolate->frame_complete_lock, {
    FrameSet_insert(&isolate->blocking_frames, frame, sizeof(Frame *));
    *VoidPtrArray_push_back_ref(&isolate->waiting_blocking_work) =
        threadpool_create_work(blocking_pool, fn, callback, fn_args);
  });
}

void isolate_remove_blocking_frame(Isolate *isolate, Frame *frame) {
  SYNCHRONIZED(isolate->frame_complete_lock, {
    FrameSet_remove(&isolate->blocking_frames, frame, sizeof(Frame *));
  });
}

void isolate_delete_frame(Isolate *isolate, Frame *frame) {
  frame_finalize(frame);
  arena_free(&isolate->frame_arena, frame);
}

void isolate_wake_dependent_frames(Isolate *isolate, Frame *frame,
                                   bool should_push) {
  // Frames in other threads may register as dependents (see
  // isolate_wait_on_frame()) while this frame completes.
  CRITICAL(frame->parent_isolate->frame_waiting_cs, {
    FrameSetIterator dependent_frames;
    FrameSet_iterator(&dependent_frames, &frame->dependent_frames);
    for (; FrameSet_has_next(&dependent_frames);
         FrameSet_next(&dependent_frames)) {
      Frame *dependent_frame = *FrameSet_mutable_value(&dependent_frames);
      // Only requeue parent task if it is waiting.
      if (FRAME_WAITING != dependent_frame->state) {
        continue;
      } else if (FRAME_ERROR == frame->state) {
        dependent_frame->child_frame_has_error = true;
      }
      Isolate *dependent_isolate = dependent_frame->parent_isolate;
      *frame_mutable_acc(dependent_frame) = *frame_get_acc(frame);
      CRITICAL(dependent_isolate->frame_waiting_cs, {
        if (should_push) {
          isolate_push_frame(dependent_isolate, dependent_frame);
        } else {
          isolate_enqueue_frame(dependent_isolate, dependent_frame);
        }
        isolate_remove_waiting_frame(dependent_isolate, dependent_frame);
      });
    }
  });
}
