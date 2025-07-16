// semaphore.c
//
// Created on: Nov 9, 2018
//     Author: Jeff Manzione

#include "util/sync/semaphore.h"

#if defined(OS_WINDOWS)
#include <windows.h>
#endif

#include "alloc/alloc.h"

Semaphore semaphore_create(uint32_t count) {
#if defined(OS_WINDOWS)
  return CreateSemaphore(NULL, 0, count, NULL);
#else
  Semaphore sem = MNEW(sem_t);
  sem_init(sem, 0, count);
  return sem;
#endif
}

WaitStatus semaphore_wait(Semaphore s) {
#if defined(OS_WINDOWS)
  return WaitForSingleObject(s, INFINITE);
#else
  return sem_wait(s);
#endif
}

void semaphore_post(Semaphore s) {
#if defined(OS_WINDOWS)
  ReleaseSemaphore(s, 1, NULL);
#else
  sem_post(s);
#endif
}

void semaphore_close(Semaphore s) {
#if defined(OS_WINDOWS)
  CloseHandle(s);
#else
  sem_destroy(s);
  RELEASE(s);
#endif
}
