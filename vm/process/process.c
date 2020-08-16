// process.c
//
// Created on: Jul 4, 2020
//     Author: Jeff Manzione

#include "vm/process/process.h"

#include "alloc/arena/arena.h"
#include "struct/struct_defaults.h"
#include "vm/process/processes.h"
#include "vm/process/task.h"

void process_init(Process *process) {
  HeapConf conf = {.mgraph_config = {.eager_delete_edges = true,
                                     .eager_delete_edges = true}};
  process->heap = heap_create(&conf);
  __arena_init(&process->task_arena, sizeof(Object), "Task");
  Q_init(&process->queued_tasks);
  set_init_default(&process->waiting_tasks);
  set_init_default(&process->completed_tasks);
}

void process_finalize(Process *process) {
  heap_delete(process->heap);
  __arena_finalize(&process->task_arena);
  Q_finalize(&process->queued_tasks);
  set_finalize(&process->waiting_tasks);
  set_finalize(&process->completed_tasks);
}

Task *process_create_task(Process *process) {
  Task *task = (Task *)__arena_alloc(&process->task_arena);
  task_init(task);
  task->parent_process = process;
  Q_enqueue(&process->queued_tasks, task);
  return task;
}