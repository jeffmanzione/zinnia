// process.c
//
// Created on: Jul 4, 2020
//     Author: Jeff Manzione

#include "vm/process/process.h"

#include "vm/process/processes.h"
#include "vm/process/task.h"

void process_init(Process *process) {
  HeapConf conf = {.mgraph_config = {.eager_delete_edges = true,
                                     .eager_delete_edges = true}};
  process->heap = heap_create(&conf);
  alist_init(&process->task_queue, Task, DEFAULT_ARRAY_SZ);
}

void process_finalize(Process *process) {
  heap_delete(process->heap);
  int i;
  for (i = 0; i < alist_len(&process->task_queue); ++i) {
    task_finalize(alist_get(&process->task_queue, i));
  }
  alist_finalize(&process->task_queue);
}

Task *process_create_task(Process *process) {
  Task *task = (Task *)alist_add(&process->task_queue);
  task_init(task);
  task->parent_process = process;
  return task;
}
