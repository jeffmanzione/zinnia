// mutex.h
//
// Created on: Nov 9, 2018
//     Author: Jeff Manzione

#ifndef UTIL_SYNC_MUTEX_H_
#define UTIL_SYNC_MUTEX_H_

#include "util/platform.h"
#include "util/sync/constants.h"

typedef struct __Condition Condition;

#ifdef OS_WINDOWS
typedef void *Mutex;
#else
#include <pthread.h>
typedef pthread_mutex_t *Mutex;
#endif

#define SYNCHRONIZED(mutex, block)                                             \
  {                                                                            \
    mutex_lock(mutex);                                                         \
    { block; }                                                                 \
    mutex_unlock(mutex);                                                       \
  }

Mutex mutex_create();
WaitStatus mutex_lock(Mutex mutex);
void mutex_unlock(Mutex mutex);
void mutex_close(Mutex mutex);

Condition *mutex_condition(Mutex mutex);
void mutex_condition_delete(Condition *cond);
void mutex_condition_broadcast(Condition *cond);
void mutex_condition_wait(Condition *cond);

#endif /* UTIL_SYNC_MUTEX_H_ */