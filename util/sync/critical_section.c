#include "util/sync/critical_section.h"

#include "alloc/alloc.h"
#include "debug/debug.h"

struct __Condition {
  CriticalSection cs;
#ifdef OS_WINDOWS
  CONDITION_VARIABLE cv;
#else
  pthread_cond_t cond;
#endif
};

CriticalSection critical_section_create() {
#ifdef OS_WINDOWS
  CriticalSection cs = CNEW(CRITICAL_SECTION);
  InitializeCriticalSection(cs);
  return cs;
#else
  return mutex_create();
#endif
}

void critical_section_enter(CriticalSection critical_section) {
#ifdef OS_WINDOWS
  EnterCriticalSection(critical_section);
#else
  mutex_lock(critical_section);
#endif
}

void critical_section_leave(CriticalSection critical_section) {
#ifdef OS_WINDOWS
  LeaveCriticalSection(critical_section);
#else
  mutex_unlock(critical_section);
#endif
}

void critical_section_delete(CriticalSection critical_section) {
#ifdef OS_WINDOWS
  DeleteCriticalSection(critical_section);
  RELEASE(critical_section);
#else
  mutex_close(critical_section);
#endif
}

Condition *critical_section_create_condition(CriticalSection critical_section) {
  Condition *cond = CNEW(Condition);
  cond->cs = critical_section;
#ifndef OS_WINDOWS
  pthread_cond_init(&cond->cond, NULL);
#endif
  return cond;
}

void condition_broadcast(Condition *cond) {
#ifdef OS_WINDOWS
  WakeAllConditionVariable(&cond->cv);
#else
  pthread_cond_broadcast(&cond->cond);
#endif
}

void condition_wait(Condition *cond) {
#ifdef OS_WINDOWS
  SleepConditionVariableCS(&cond->cv, cond->cs, INFINITE);
#else
  pthread_cond_wait(&cond->cond, &cond->cs->lock);
#endif
}

void condition_delete(Condition *cond) {
#ifndef OS_WINDOWS
  pthread_cond_destroy(&cond->cond);
#endif
  RELEASE(cond);
}