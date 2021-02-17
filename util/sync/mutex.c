// mutex.h
//
// Created on: Nov 9, 2018
//     Author: Jeff Manzione

#include "util/sync/mutex.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#ifdef OS_WINDOWS
#include <process.h>
#include <windows.h>
#endif

#include "alloc/alloc.h"
#include "debug/debug.h"

inline Mutex mutex_create() {
#ifdef OS_WINDOWS
  return CreateMutex(NULL, false, NULL);
#else
  Mutex mutex = ALLOC2(pthread_mutex_t);
  pthread_mutex_init(mutex, NULL);
  return mutex;
#endif
}

inline WaitStatus mutex_lock(Mutex mutex) {
#ifdef OS_WINDOWS
  return WaitForSingleObject(mutex, INFINITE);
#else
  return pthread_mutex_lock(mutex);
#endif
}

inline void mutex_unlock(Mutex mutex) {
#ifdef OS_WINDOWS
  ReleaseMutex(mutex);
#else
  pthread_mutex_unlock(mutex);
#endif
}

inline void mutex_close(Mutex mutex) {
#ifdef OS_WINDOWS
  CloseHandle(mutex);
#else
  pthread_mutex_destroy(mutex);
  DEALLOC(mutex);
#endif
}

struct __Condition {
#if !defined(OS_WINDOWS)
  pthread_cond_t cond;
#endif
  Mutex mutex;
};

inline Condition *mutex_condition(Mutex mutex) {
  Condition *cond = ALLOC2(Condition);
  cond->mutex = mutex;
#ifdef OS_WINDOWS
  ERROR("Unimplemented.");
#else
  pthread_cond_init(&cond->cond, NULL);
#endif
  return cond;
}

inline void mutex_condition_delete(Condition *cond) {
#ifdef OS_WINDOWS
  ERROR("Unimplemented.");
#else
  pthread_cond_destroy(&cond->cond);
  DEALLOC(cond);
#endif
}

inline void mutex_condition_broadcast(Condition *cond) {
#ifdef OS_WINDOWS
  ERROR("Unimplemented.");
#else
  pthread_cond_broadcast(&cond->cond);
#endif
}

inline void mutex_condition_wait(Condition *cond) {
#ifdef OS_WINDOWS
  ERROR("Unimplemented.");
#else
  pthread_cond_wait(&cond->cond, cond->mutex);
#endif
}
