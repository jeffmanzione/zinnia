#include "util/sync/threadpool.h"

#include "alloc/alloc.h"
#include "struct/q.h"
#include "util/sync/mutex.h"
#include "util/sync/semaphore.h"
#include "util/sync/thread.h"

struct __Work {
  VoidFnPtr fn;
  VoidFnPtr callback;
  VoidPtr fn_args;
};

struct __ThreadPool {
  size_t num_threads;
  ThreadHandle *threads;

  Semaphore thread_sem;
  Mutex work_mutex;
  Q work;
};

void _do_work(ThreadPool *tp) {
  Work *w = NULL;
  for (;;) {
    semaphore_wait(tp->thread_sem);
    SYNCHRONIZED(tp->work_mutex, {
      if (!Q_is_empty(&tp->work)) {
        w = (Work *)Q_pop(&tp->work);
      } else {
        semaphore_post(tp->thread_sem);
      }
    });
    if (NULL != w) {
      w->fn(w->fn_args);
      w->callback(w->fn_args);
      DEALLOC(w);
      w = NULL;
    }
  }
}

ThreadPool *threadpool_create(size_t num_threads) {
  ThreadPool *tp = ALLOC2(ThreadPool);
  Q_init(&tp->work);

  tp->num_threads = num_threads;
  tp->work_mutex = mutex_create();
  tp->thread_sem = semaphore_create(0);
  tp->threads = ALLOC_ARRAY2(ThreadHandle, num_threads);
  int i;
  for (i = 0; i < num_threads; ++i) {
    tp->threads[i] = thread_create((VoidFn)_do_work, (VoidPtr)tp);
  }
  return tp;
}

void threadpool_delete(ThreadPool *tp) {
  Q_finalize(&tp->work);
  mutex_close(tp->work_mutex);
  semaphore_close(tp->thread_sem);
  int i;
  for (i = 0; i < tp->num_threads; ++i) {
    thread_close(tp->threads[i]);
  }
  DEALLOC(tp->threads);
  DEALLOC(tp);
}

void threadpool_execute(ThreadPool *tp, VoidFnPtr fn, VoidFnPtr callback,
                        VoidPtr fn_args) {
  Work *w = threadpool_create_work(tp, fn, callback, fn_args);
  threadpool_execute_work(tp, w);
}

Work *threadpool_create_work(ThreadPool *tp, VoidFnPtr fn, VoidFnPtr callback,
                             VoidPtr fn_args) {
  Work *w = ALLOC2(Work);
  w->fn = fn;
  w->callback = callback;
  w->fn_args = fn_args;
  return w;
}

void threadpool_execute_work(ThreadPool *tp, Work *w) {
  SYNCHRONIZED(tp->work_mutex, {
    *Q_add_last(&tp->work) = w;
    semaphore_post(tp->thread_sem);
  });
}