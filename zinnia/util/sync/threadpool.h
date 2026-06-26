#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_SYNC_THREADPOOL_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_SYNC_THREADPOOL_H_

#include <stdlib.h>

typedef void (*VoidFnPtr)(void *);
typedef void *VoidPtr;

typedef struct ThreadPool__ ThreadPool;
typedef struct Work__ Work;

ThreadPool *threadpool_create(size_t num_threads);
void threadpool_delete(ThreadPool *threadpool);
void threadpool_execute(ThreadPool *threadpool, VoidFnPtr fn,
                        VoidFnPtr callback, VoidPtr args);
Work *threadpool_create_work(ThreadPool *tp, VoidFnPtr fn, VoidFnPtr callback,
                             VoidPtr fn_args);
void threadpool_execute_work(ThreadPool *tp, Work *w);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_SYNC_THREADPOOL_H_ */