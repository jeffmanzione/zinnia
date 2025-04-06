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
  Mutex mutex = MNEW(_Mutex);
  pthread_mutexattr_init(&mutex->attr);
  pthread_mutexattr_settype(&mutex->attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&mutex->lock, &mutex->attr);
  return mutex;
#endif
}

WaitStatus mutex_lock(Mutex mutex) {
#ifdef OS_WINDOWS
  return WaitForSingleObject(mutex, INFINITE);
#else
  return pthread_mutex_lock(&mutex->lock);
#endif
}

void mutex_unlock(Mutex mutex) {
#ifdef OS_WINDOWS
  ReleaseMutex(mutex);
#else
  pthread_mutex_unlock(&mutex->lock);
#endif
}

void mutex_close(Mutex mutex) {
#ifdef OS_WINDOWS
  CloseHandle(mutex);
#else
  pthread_mutex_destroy(&mutex->lock);
  RELEASE(mutex);
#endif
}
