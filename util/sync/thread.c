// thread.h
//
// Created on: Nov 9, 2018
//     Author: Jeff Manzione

#include "util/sync/thread.h"

#include <stdlib.h>

#ifdef OS_WINDOWS
#include <process.h>
#include <windows.h>
#endif

ThreadHandle thread_create(VoidFn fn, void *arg) {
#ifdef OS_WINDOWS
  ThreadId id;
  return _beginthreadex(NULL, 0, fn, arg, 0, &id);
#else
  ThreadHandle thread;
  pthread_create(&thread, NULL, fn, arg);
  return thread;
#endif
}

WaitStatus thread_join(ThreadHandle thread, unsigned long duration) {
#ifdef OS_WINDOWS
  return WaitForSingleObject((HANDLE)thread, duration);
#else
  return pthread_join(thread, NULL);
#endif
}

void thread_close(ThreadHandle thread) {
#ifdef OS_WINDOWS
  CloseHandle((HANDLE)thread);
#else
  pthread_cancel(thread);
#endif
}
