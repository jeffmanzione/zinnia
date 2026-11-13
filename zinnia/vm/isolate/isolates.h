// process.h
//
// Created on: Jul 7, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_ISOLATE_ISOLATES_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_ISOLATE_ISOLATES_H_

#include "c-data-structures/arraylike.h"
#include "c-data-structures/setlike.h"
#include "rzalloc/rzalloc.h"
#include "zinnia/heap/heap.h"
#include "zinnia/program/bytecode.h"
#include "zinnia/util/sync/critical_section.h"
#include "zinnia/util/sync/mutex.h"
#include "zinnia/util/sync/thread.h"
#include "zinnia/util/sync/threadpool.h"
#include "zinnia/value/module/module.h"
#include "zinnia/value/object.h"

typedef struct _VM VM;
typedef struct __Scope Scope;
typedef struct __Frame Frame;
typedef struct __Isolate Isolate;

// Handles a message posted to isolate. Runs on isolate's thread while holding
// its heap lock.
typedef void (*IsolateMessageHandler)(Isolate *isolate, void *data);
typedef struct Future_ Future;

uint32_t hash_frame(const Frame *tsk, uint32_t size);
int32_t compare_frames(const Frame *tsk1, uint32_t size1, const Frame *tsk2,
                       uint32_t size2);

DEFINE_ARRAYLIKE(ValueStack, Value);
DEFINE_SETLIKE(FrameSet, Frame *);
DEFINE_ARRAYLIKE(FrameArray, Frame *);

struct __Scope {
  Frame *parent_frame;
  // The isolate whose heap the scope is in. A scope captured by an anonymous
  // function can outlive its frame, so this doesn't go through parent_frame.
  Isolate *parent_isolate;
  // The scope the scope is nested in, where names are looked up next.
  Scope *previous_scope;
  // For a function or method called in its caller's frame: the scope that
  // called it, which runs again when it returns, and the size of the frame's
  // value stack when it was called.
  Scope *caller_scope;
  uint32_t caller_stack_size;

  Object *_reflection;

  Value self;
  Module *module;
  const Bytecode *bytecode;
  const Function *func;
  uint32_t ins;

  Object *error;
  int32_t catch_ins;

  // Interned names of the variables declared const in this scope, or NULL.
  const char **const_ids;
  uint16_t num_const_ids;
  uint16_t const_ids_capacity;
};

typedef enum {
  FRAME_NEW,
  FRAME_RUNNING,
  FRAME_WAITING,
  FRAME_COMPLETE,
  FRAME_ERROR,
} FrameState;

typedef enum {
  NOT_WAITING,
  WAITING_TO_START,
  WAITING_ON_FN_CALL,
  WAITING_ON_FUTURE,
  // Waiting for the main isolate to initialize a module.
  WAITING_ON_MODULE,
} WaitReason;

struct __Frame {
  volatile FrameState state;
  WaitReason wait_reason;

  Isolate *parent_isolate;
  Scope *current;

  Value acc;
  ValueStack value_stack;

  Frame *parent_frame;

  FrameSet dependent_frames;

  bool child_frame_has_error;
  bool is_finalized;

  Object *_reflection;

  Future *actor_future;
  // The future to the frame's result, if any, until the frame completes.
  Object *future;
  // The module whose top-level code this frame runs, if any.
  Module *initializing_module;
};

struct __Isolate {
  VM *vm;
  Heap *heap;

  RzallocArena frame_arena;
  RzallocArena scope_arena;
  Mutex frame_create_lock;
  Mutex frame_queue_lock;

  CriticalSection frame_waiting_cs;
  Condition *frame_wait_cond;

  Mutex frame_complete_lock;

  Frame *current_frame;
  FrameArray queued_frames;
  FrameSet waiting_frames;
  FrameSet completed_frames;
  FrameSet blocking_frames;

  Mutex heap_access_lock;
  // Instructions run since the last check for garbage collection.
  uint32_t instructions_since_gc_check;

  Object *_reflection;
  ThreadHandle thread;  // Null if main thread.

  VoidPtrArray waiting_blocking_work;
  // IsolateMessages posted by other threads, guarded by frame_waiting_cs.
  // Other isolates must never touch this isolate's heap, so they post work
  // that needs it here to be done on this isolate's thread.
  VoidPtrArray messages;

  Future *future;
  bool keep_alive;
  Frame *keep_alive_frame;
  // The frame running the function the isolate was spawned with, and its
  // result (or error, if failed) once it completes, which completes future.
  Frame *start_frame;
  Value result;
  bool failed;
};

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_ISOLATE_ISOLATES_H_ */