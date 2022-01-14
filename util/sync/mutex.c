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

Mutex mutex_create() {
#ifdef OS_WINDOWS
  return CreateMutex(NULL, false, NULL);
#else
  Mutex mutex = ALLOC2(pthread_mutex_t);
  pthread_mutex_init(mutex, NULL);
  return mutex;
#endif
}

WaitStatus mutex_lock(Mutex mutex) {
#ifdef OS_WINDOWS
  return WaitForSingleObject(mutex, INFINITE);
#else
  return pthread_mutex_lock(mutex);
#endif
}

void mutex_unlock(Mutex mutex) {
#ifdef OS_WINDOWS
  ReleaseMutex(mutex);
#else
  pthread_mutex_unlock(mutex);
#endif
}

void mutex_close(Mutex mutex) {
#ifdef OS_WINDOWS
  CloseHandle(mutex);
#else
  pthread_mutex_destroy(mutex);
  DEALLOC(mutex);
#endif
}
