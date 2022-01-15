// process.c
//
// Created on: Jul 4, 2020
//     Author: Jeff Manzione

#include "vm/process/process.h"

#include "alloc/arena/arena.h"
#include "entity/class/classes.h"
#include "struct/struct_defaults.h"
#include "util/sync/mutex.h"
#include "vm/process/processes.h"
#include "vm/process/task.h"

void process_init(Process *process) {
  HeapConf conf = {.mgraph_config = {.eager_delete_edges = true,
                                     .eager_delete_edges = true}};
  process->heap = heap_create(&conf);
  __arena_init(&process->task_arena, sizeof(Task), "Task");
  __arena_init(&process->context_arena, sizeof(Context), "Context");
  process->task_create_lock = mutex_create();
  process->task_queue_lock = mutex_create();
  process->task_waiting_cs = critical_section_create();
  process->task_wait_cond =
      critical_section_create_condition(process->task_waiting_cs);
  process->task_complete_lock = mutex_create();
  Q_init(&process->queued_tasks);
  set_init_default(&process->waiting_tasks);
  set_init_default(&process->completed_tasks);
  process->_reflection = NULL;
  process->waiting_background_work = NULL;
}

void process_finalize(Process *process) {
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
  heap_delete(process->heap);
  mutex_close(process->task_create_lock);
  mutex_close(process->task_queue_lock);
  condition_delete(process->task_wait_cond);
  critical_section_delete(process->task_waiting_cs);
  mutex_close(process->task_complete_lock);
}

void _task_add_reflection(Process *process, Task *task) {
  task->_reflection = heap_new(process->heap, Class_Task);
  task->_reflection->_internal_obj = task;
}

Task *process_create_unqueued_task(Process *process) {
  Task *task;
  SYNCHRONIZED(process->task_create_lock, {
    task = (Task *)__arena_alloc(&process->task_arena);
    task_init(task);
    task->parent_process = process;
    _task_add_reflection(process, task);
    heap_inc_edge(process->heap, process->_reflection, task->_reflection);
  });
  return task;
}

Task *process_create_task(Process *process) {
  Task *task = process_create_unqueued_task(process);
  process_enqueue_task(process, task);
  return task;
}

Task *process_pop_task(Process *process) {
  Task *task;
  SYNCHRONIZED(process->task_queue_lock, {
    if (0 == Q_size(&process->queued_tasks)) {
      task = NULL;
    } else {
      task = Q_pop(&process->queued_tasks);
    }
  });
  return task;
}

void process_enqueue_task(Process *process, Task *task) {
  SYNCHRONIZED(process->task_queue_lock,
               { *Q_add_last(&process->queued_tasks) = task; });
}

size_t process_queue_size(Process *process) {
  size_t size;
  SYNCHRONIZED(process->task_queue_lock,
               { size = Q_size(&process->queued_tasks); });
  return size;
}

void process_insert_waiting_task(Process *process, Task *task) {
  CRITICAL(process->task_waiting_cs,
           { set_insert(&process->waiting_tasks, task); });
}

void process_remove_waiting_task(Process *process, Task *task) {
  CRITICAL(process->task_waiting_cs,
           { set_remove(&process->waiting_tasks, task); });
}

void process_mark_task_complete(Process *process, Task *task) {
  SYNCHRONIZED(process->task_complete_lock,
               { set_insert(&process->completed_tasks, task); });
}