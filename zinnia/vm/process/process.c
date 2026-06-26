// process.c
//
// Created on: Jul 4, 2020
//     Author: Jeff Manzione

#include "zinnia/vm/process/process.h"

#include "rzalloc/rzalloc.h"
#include "zinnia/entity/class/classes_def.h"
#include "zinnia/util/sync/mutex.h"
#include "zinnia/vm/process/processes.h"
#include "zinnia/vm/process/task.h"


void process_init(Process *process, HeapConf *conf) {
  process->heap = heap_create(conf);
  arena_init(&process->task_arena, sizeof(Task));
  arena_init(&process->context_arena, sizeof(Context));
  process->task_create_lock = mutex_create();
  process->task_queue_lock = mutex_create();
  process->heap_access_lock = mutex_create();
  process->task_waiting_cs = critical_section_create();
  process->task_wait_cond =
      critical_section_create_condition(process->task_waiting_cs);
  process->task_complete_lock = mutex_create();
  TaskArray_init(&process->queued_tasks);
  TaskSet_init(&process->waiting_tasks, hash_task, compare_tasks);
  TaskSet_init(&process->completed_tasks, hash_task, compare_tasks);
  TaskSet_init(&process->background_tasks, hash_task, compare_tasks);
  process->_reflection = NULL;
  VoidPtrArray_init(&process->waiting_background_work);
  process->future = NULL;
  process->is_remote = false;
  process->remote_non_daemon_task = NULL;
}

void process_finalize(Process *process) {
  TaskArrayIterator q_iter;
  TaskArray_iterator(&q_iter, &process->queued_tasks);
  for (; TaskArray_has_next(&q_iter); TaskArray_next(&q_iter)) {
    task_finalize(*TaskArray_mutable_value(&q_iter));
  }
  TaskArray_finalize(&process->queued_tasks);

  TaskSetIterator m_iter;
  TaskSet_iterator(&m_iter, &process->waiting_tasks);
  for (; TaskSet_has_next(&m_iter); TaskSet_next(&m_iter)) {
    task_finalize(*TaskSet_mutable_value(&m_iter));
  }
  TaskSet_finalize(&process->waiting_tasks);

  TaskSet_iterator(&m_iter, &process->completed_tasks);
  for (; TaskSet_has_next(&m_iter); TaskSet_next(&m_iter)) {
    task_finalize(*TaskSet_mutable_value(&m_iter));
  }
  TaskSet_finalize(&process->completed_tasks);

  // Is this necessary?
  TaskSet_iterator(&m_iter, &process->background_tasks);
  for (; TaskSet_has_next(&m_iter); TaskSet_next(&m_iter)) {
    task_finalize(*TaskSet_mutable_value(&m_iter));
  }
  TaskSet_finalize(&process->background_tasks);

  arena_clear(&process->task_arena);
  arena_clear(&process->context_arena);
  heap_delete(process->heap);
  mutex_close(process->task_create_lock);
  mutex_close(process->task_queue_lock);
  mutex_close(process->heap_access_lock);
  condition_delete(process->task_wait_cond);
  critical_section_delete(process->task_waiting_cs);
  mutex_close(process->task_complete_lock);
  VoidPtrArray_finalize(&process->waiting_background_work);
}

void task_add_reflection_(Process *process, Task *task) {
  task->_reflection = heap_new(process->heap, Class_Task);
  task->_reflection->_internal_obj = task;
}

Task *process_create_unqueued_task(Process *process) {
  Task *task;
  SYNCHRONIZED(process->task_create_lock, {
    task = (Task *)arena_malloc(&process->task_arena);
    task_init(task);
    task->parent_process = process;
    task_add_reflection_(process, task);
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
    if (TaskArray_is_empty(&process->queued_tasks)) {
      task = NULL;
    } else {
      task = TaskArray_pop_front_unchecked(&process->queued_tasks);
    }
  });

  // Task can be complete if it was prioritized to the front of the queue.
  // This happens to a parent task when a child has an error.
  if (NULL != task && TASK_COMPLETE == task->state) {
    return process_pop_task(process);
  }
  return task;
}

void process_enqueue_task(Process *process, Task *task) {
  SYNCHRONIZED(process->task_queue_lock,
               { *TaskArray_push_back_ref(&process->queued_tasks) = task; });
}

void process_push_task(Process *process, Task *task) {
  SYNCHRONIZED(process->task_queue_lock,
               { TaskArray_push_back(&process->queued_tasks, task); });
}

size_t process_queue_size(Process *process) {
  size_t size;
  SYNCHRONIZED(process->task_queue_lock,
               { size = TaskArray_size(&process->queued_tasks); });
  return size;
}

void process_insert_waiting_task(Process *process, Task *task) {
  task->state = TASK_WAITING;
  CRITICAL(process->task_waiting_cs,
           { TaskSet_insert(&process->waiting_tasks, task, sizeof(Task *)); });
}

void process_remove_waiting_task(Process *process, Task *task) {
  CRITICAL(process->task_waiting_cs,
           { TaskSet_remove(&process->waiting_tasks, task, sizeof(Task *)); });
}

void process_mark_task_complete(Process *process, Task *task) {
  SYNCHRONIZED(process->task_complete_lock, {
    TaskSet_insert(&process->completed_tasks, task, sizeof(Task *));
  });
}

void process_add_background_task(Process *process, Task *task,
                                 ThreadPool *background_pool, VoidFnPtr fn,
                                 VoidFnPtr callback, VoidPtr fn_args) {
  SYNCHRONIZED(process->task_complete_lock, {
    TaskSet_insert(&process->background_tasks, task, sizeof(Task *));
    *VoidPtrArray_push_back_ref(&process->waiting_background_work) =
        threadpool_create_work(background_pool, fn, callback, fn_args);
  });
}

void process_remove_background_task(Process *process, Task *task) {
  SYNCHRONIZED(process->task_complete_lock, {
    TaskSet_remove(&process->background_tasks, task, sizeof(Task *));
  });
}

void process_delete_task(Process *process, Task *task) {
  task_finalize(task);
  arena_free(&process->task_arena, task);
}