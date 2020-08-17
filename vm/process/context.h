// context.h
//
// Created on: Jun 13, 2020
//     Author: Jeff Manzione

#ifndef VM_PROCESS_CONTEXT_H_
#define VM_PROCESS_CONTEXT_H_

#include "entity/module/module.h"
#include "entity/object.h"
#include "program/instruction.h"
#include "vm/process/processes.h"

void context_init(Context *ctx, Object *self, Object *member_obj,
                  Module *module, uint32_t instruction_pos);
void context_finalize(Context *ctx);
Object *context_self(Context *ctx);
Module *context_module(Context *ctx);
const Instruction *context_ins(Context *ctx);

Entity *context_lookup(Context *ctx, const char id[]);
void context_let(Context *ctx, const char id[], const Entity *e);
void context_set(Context *ctx, const char id[], const Entity *e);

Context *task_get_context_for_index(Task *task, uint32_t index);

#endif /* VM_PROCESS_CONTEXT_H_ */