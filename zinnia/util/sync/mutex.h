// mutex.h
//
// Created on: Nov 9, 2018
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_SYNC_MUTEX_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_SYNC_MUTEX_H_

#include "zinnia/util/platform.h"
#include "zinnia/util/sync/constants.h"

#ifdef OS_WINDOWS
typedef void *Mutex;
#else
#include <pthread.h>
typedef struct {
  pthread_mutexattr_t attr;
  pthread_mutex_t lock;
} _Mutex;
typedef _Mutex *Mutex;
#endif

#define SYNCHRONIZED(mutex, block) \
  {                                \
    mutex_lock(mutex);             \
    { block; }                     \
    mutex_unlock(mutex);           \
  }

Mutex mutex_create();
WaitStatus mutex_lock(Mutex mutex);
void mutex_unlock(Mutex mutex);
void mutex_close(Mutex mutex);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_SYNC_MUTEX_H_ */