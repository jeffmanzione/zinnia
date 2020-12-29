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

Mutex mutex_create() {
#ifdef OS_WINDOWS
  return CreateMutex(NULL, false, NULL);
#else
  Mutex lock = ALLOC2(pthread_mutex_t);
  return pthread_mutex_init(lock, NULL);
#endif
}

WaitStatus mutex_await(Mutex id, unsigned long duration) {
#ifdef OS_WINDOWS
  return WaitForSingleObject(id, duration);
#else
  return pthread_mutex_lock(id);
#endif
}

void mutex_release(Mutex id) {
#ifdef OS_WINDOWS
  ReleaseMutex(id);
#else
  pthread_mutex_unlock(id);
#endif
}

void mutex_close(Mutex id) {
#ifdef OS_WINDOWS
  CloseHandle(id);
#else
  pthread_mutex_destroy(id);
  DEALLOC(id);
#endif
}
