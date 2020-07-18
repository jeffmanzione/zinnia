// process.h
//
// Created on: Jul 3, 2020
//     Author: Jeff Manzione

#ifndef VM_PROCESS_PROCESS_H_
#define VM_PROCESS_PROCESS_H_

#include "vm/process/processes.h"

void process_init(Process *process);
void process_finalize(Process *process);
Task *process_create_task(Process *process);

#endif /* VM_PROCESS_PROCESS_H_ */