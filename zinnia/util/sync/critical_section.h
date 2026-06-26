// critical_section.h
//
// Created on: Jan 9, 2022
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_SYNC_CRITICAL_SECTION_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_SYNC_CRITICAL_SECTION_H_

#include "zinnia/util/platform.h"
#include "zinnia/util/sync/constants.h"

#ifdef OS_WINDOWS
#include <windows.h>
#endif

#ifdef OS_WINDOWS
#include <synchapi.h>
typedef CRITICAL_SECTION *CriticalSection;
#else
#include "zinnia/util/sync/mutex.h"
typedef Mutex CriticalSection;
#endif

#define CRITICAL(cs, block)     \
  {                             \
    critical_section_enter(cs); \
    { block; }                  \
    critical_section_leave(cs); \
  }

CriticalSection critical_section_create();
void critical_section_enter(CriticalSection critical_section);
void critical_section_leave(CriticalSection critical_section);
void critical_section_delete(CriticalSection critical_section);

typedef struct __Condition Condition;

Condition *critical_section_create_condition(CriticalSection critical_section);
void condition_broadcast(Condition *cond);
void condition_wait(Condition *cond);
void condition_delete(Condition *cond);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_SYNC_CRITICAL_SECTION_H_ */