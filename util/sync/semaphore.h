// semaphore.h
//
// Created on: Nov 9, 2018
//     Author: Jeff Manzione

#ifndef UTIL_SYNC_SEMAPHORE_H_
#define UTIL_SYNC_SEMAPHORE_H_

#include <stdint.h>
#include <stdio.h>

#include "util/platform.h"
#include "util/sync/constants.h"

#if defined(OS_WINDOWS)
typedef void *Semaphore;
#else
#include <semaphore.h>
#include <unistd.h>
typedef sem_t *Semaphore;
#endif

Semaphore semaphore_create(uint32_t count);
WaitStatus semaphore_wait(Semaphore s);
void semaphore_post(Semaphore s);
void semaphore_close(Semaphore s);

#endif /* UTIL_SYNC_SEMAPHORE_H_ */