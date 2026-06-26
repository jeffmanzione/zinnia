// thread.h
//
// Created on: Nov 9, 2018
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_SYNC_THREAD_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_SYNC_THREAD_H_

#include "zinnia/util/platform.h"
#include "zinnia/util/sync/constants.h"

typedef unsigned int ThreadId;

#ifdef OS_WINDOWS
#include <stdint.h>

typedef uintptr_t ThreadHandle;
typedef unsigned (*VoidFn)(void *);
#else
#include <pthread.h>
typedef pthread_t ThreadHandle;
typedef void *(*VoidFn)(void *);
#endif

#define AS_VOID_FN(fn) ((VoidFn)fn)

ThreadHandle thread_create(VoidFn fn, void *arg);
WaitStatus thread_join(ThreadHandle thread, unsigned long duration);
void thread_close(ThreadHandle thread);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_SYNC_THREAD_H_ */