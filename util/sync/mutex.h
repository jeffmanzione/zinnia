// mutex.h
//
// Created on: Nov 9, 2018
//     Author: Jeff Manzione

#ifndef UTIL_SYNC_MUTEX_H_
#define UTIL_SYNC_MUTEX_H_

#include "util/platform.h"

#ifdef OS_WINDOWS
typedef void *Mutex;
#else
#include <pthread.h>
typedef pthread_mutex_t *Mutex;
#endif

typedef unsigned long WaitStatus;

Mutex mutex_create();
WaitStatus mutex_await(Mutex id, unsigned long duration);
void mutex_release(Mutex id);
void mutex_close(Mutex id);

#endif /* UTIL_SYNC_MUTEX_H_ */