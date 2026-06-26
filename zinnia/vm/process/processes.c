#include "zinnia/vm/process/processes.h"

IMPL_ARRAYLIKE(EntityStack, Entity);
IMPL_SETLIKE(TaskSet, Task *);
IMPL_ARRAYLIKE(TaskArray, Task *);

uint32_t hash_task(const Task *tsk, uint32_t size) {
  return (uint32_t)(intptr_t)tsk;
}

int32_t compare_tasks(const Task *tsk1, uint32_t size1, const Task *tsk2,
                      uint32_t size2) {
  return (intptr_t)tsk1 - (intptr_t)tsk2;
}