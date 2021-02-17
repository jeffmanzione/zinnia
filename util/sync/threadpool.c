#include "util/sync/threadpool.h"

#include "alloc/alloc.h"
#include "struct/q.h"
#include "util/sync/mutex.h"
#include "util/sync/semaphore.h"
#include "util/sync/thread.h"

typedef struct {
  VoidFnPtr fn;
  VoidFnPtr callback;
  VoidPtr fn_args;
} _Work;

struct __ThreadPool {
  size_t num_threads;
  ThreadHandle *threads;

  Semaphore thread_sem;
  Mutex work_mutex;
  Q work;
};

void _do_work(ThreadPool *tp) {
  _Work *w = NULL;
  for (;;) {
    semaphore_wait(tp->thread_sem);
    SYNCHRONIZED(tp->work_mutex, {
      if (!Q_is_empty(&tp->work)) {
        w = (_Work *)Q_pop(&tp->work);
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
  int i;
  for (i = 0; i < tp->num_threads; ++i) {
    thread_close(tp->threads[i]);
  }
  DEALLOC(tp);
}

void threadpool_execute(ThreadPool *tp, VoidFnPtr fn, VoidFnPtr callback,
                        VoidPtr fn_args) {
  _Work *w = ALLOC2(_Work);
  w->fn = fn;
  w->callback = callback;
  w->fn_args = fn_args;
  SYNCHRONIZED(tp->work_mutex, {
    *Q_add_last(&tp->work) = w;
    semaphore_post(tp->thread_sem);
  });
}
