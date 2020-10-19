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
  __arena_init(&process->task_arena, sizeof(Task), "Task");
  __arena_init(&process->context_arena, sizeof(Context), "Context");
  Q_init(&process->queued_tasks);
  set_init_default(&process->waiting_tasks);
  set_init_default(&process->completed_tasks);
}

void process_finalize(Process *process) {
  heap_delete(process->heap);

  Q_iter q_iter = Q_iterator(&process->queued_tasks);
  for (; Q_has(&q_iter); Q_inc(&q_iter)) {
    task_finalize(*(Task **)Q_value(&q_iter));
  }
  Q_finalize(&process->queued_tasks);

  M_iter m_iter = set_iter(&process->waiting_tasks);
  for (; has(&m_iter); inc(&m_iter)) {
    task_finalize((Task *)value(&m_iter));
  }
  set_finalize(&process->waiting_tasks);

  m_iter = set_iter(&process->completed_tasks);
  for (; has(&m_iter); inc(&m_iter)) {
    task_finalize((Task *)value(&m_iter));
  }
  set_finalize(&process->completed_tasks);
  __arena_finalize(&process->task_arena);
  __arena_finalize(&process->context_arena);
}

Task *process_create_task(Process *process) {
  Task *task = (Task *)__arena_alloc(&process->task_arena);
  task_init(task);
  task->parent_process = process;
  *Q_add_last(&process->queued_tasks) = task;
  return task;
}

inline Task *process_last_task(Process *process) {
  return Q_get(&process->queued_tasks, Q_size(&process->queued_tasks) - 1);
}