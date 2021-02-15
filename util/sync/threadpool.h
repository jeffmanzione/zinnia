#ifndef UTIL_SYNC_THREADPOOL_H_
#define UTIL_SYNC_THREADPOOL_H_

#include <stdlib.h>

typedef void (*VoidFnPtr)(void *);
typedef void *VoidPtr;

typedef struct __ThreadPool ThreadPool;

ThreadPool *threadpool_create(size_t num_threads);
void threadpool_delete(ThreadPool *threadpool);
void threadpool_execute(ThreadPool *threadpool, VoidFnPtr fn,
                        VoidFnPtr callback, VoidPtr args);

#endif /* UTIL_SYNC_THREADPOOL_H_ */