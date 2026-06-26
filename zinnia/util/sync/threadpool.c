#include "zinnia/util/sync/threadpool.h"

#include "zinnia/alloc/alloc.h"
#include "zinnia/util/sync/critical_section.h"
#include "zinnia/util/sync/thread.h"
#include "zinnia/util/void_array.h"

struct Work__ {
  VoidFnPtr fn;
  VoidFnPtr callback;
  VoidPtr fn_args;
};

struct ThreadPool__ {
  size_t num_threads;
  ThreadHandle *threads;
  CriticalSection work_mutex;
  Condition *work_cond;
  VoidPtrArray work;
};

void do_work_(ThreadPool *tp) {
  Work *w = NULL;
  for (;;) {
    CRITICAL(tp->work_mutex, {
      while (VoidPtrArray_is_empty(&tp->work)) {
        condition_wait(tp->work_cond);
      }
      w = (Work *)VoidPtrArray_pop_front_unchecked(&tp->work);
    });
    if (NULL != w) {
      w->fn(w->fn_args);
      w->callback(w->fn_args);
      RELEASE(w);
      w = NULL;
    }
  }
}

ThreadPool *threadpool_create(size_t num_threads) {
  ThreadPool *tp = MNEW(ThreadPool);
  VoidPtrArray_init(&tp->work);

  tp->num_threads = num_threads;
  tp->work_mutex = critical_section_create();
  tp->work_cond = critical_section_create_condition(tp->work_mutex);
  tp->threads = MNEW_ARR(ThreadHandle, num_threads);
  int i;
  for (i = 0; i < num_threads; ++i) {
    tp->threads[i] = thread_create((VoidFn)do_work_, (VoidPtr)tp);
  }
  return tp;
}

void threadpool_delete(ThreadPool *tp) {
  VoidPtrArray_finalize(&tp->work);
  critical_section_delete(tp->work_mutex);
  condition_delete(tp->work_cond);
  int i;
  for (i = 0; i < tp->num_threads; ++i) {
    thread_close(tp->threads[i]);
  }
  RELEASE(tp->threads);
  RELEASE(tp);
}

void threadpool_execute(ThreadPool *tp, VoidFnPtr fn, VoidFnPtr callback,
                        VoidPtr fn_args) {
  Work *w = threadpool_create_work(tp, fn, callback, fn_args);
  threadpool_execute_work(tp, w);
}

Work *threadpool_create_work(ThreadPool *tp, VoidFnPtr fn, VoidFnPtr callback,
                             VoidPtr fn_args) {
  Work *w = MNEW(Work);
  w->fn = fn;
  w->callback = callback;
  w->fn_args = fn_args;
  return w;
}

void threadpool_execute_work(ThreadPool *tp, Work *w) {
  CRITICAL(tp->work_mutex, { *VoidPtrArray_push_back_ref(&tp->work) = w; });
  condition_broadcast(tp->work_cond);
}