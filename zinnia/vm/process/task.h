// task.h
//
// Created on: Jul 3, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_PROCESS_TASK_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_PROCESS_TASK_H_

#include "zinnia/entity/entity.h"
#include "zinnia/vm/process/processes.h"

const char *task_state_str(TaskState state);
const char *wait_reason_str(WaitReason reason);

void task_init(Task *task);
void task_finalize(Task *task);
Context *task_create_context(Task *task, Object *self, Module *module,
                             uint32_t instruction_pos);
Context *task_back_context(Task *task);

Entity task_popstack(Task *task);
const Entity *task_peekstack(Task *task);
const Entity *task_peekstack_n(Task *task, int n);
void task_dropstack(Task *task);
Entity *task_pushstack(Task *task);

const Entity *task_get_resval(Task *task);
Entity *task_mutable_resval(Task *task);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_PROCESS_TASK_H_ */