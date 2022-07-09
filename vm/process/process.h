// process.h
//
// Created on: Jul 3, 2020
//     Author: Jeff Manzione

#ifndef VM_PROCESS_PROCESS_H_
#define VM_PROCESS_PROCESS_H_

#include "vm/process/processes.h"

void process_init(Process *process, HeapConf *config);
void process_finalize(Process *process);
Task *process_create_unqueued_task(Process *process);
Task *process_create_task(Process *process);

Task *process_pop_task(Process *process);
void process_enqueue_task(Process *process, Task *task);
size_t process_queue_size(Process *process);

void process_insert_waiting_task(Process *process, Task *task);
void process_remove_waiting_task(Process *process, Task *task);
void process_mark_task_complete(Process *process, Task *task);

void process_delete_task(Process *process, Task *task);

#endif /* VM_PROCESS_PROCESS_H_ */