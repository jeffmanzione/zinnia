// context.h
//
// Created on: Jun 13, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_PROCESS_CONTEXT_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_PROCESS_CONTEXT_H_

#include "zinnia/entity/module/module.h"
#include "zinnia/entity/object.h"
#include "zinnia/program/instruction.h"
#include "zinnia/vm/process/processes.h"

void context_init(Context *ctx, Object *self, Module *module,
                  uint32_t instruction_pos);
void context_finalize(Context *ctx);
Object *context_self(Context *ctx);
Module *context_module(Context *ctx);
const Instruction *context_ins(Context *ctx);
void context_set_function(Context *ctx, const Function *func);
Entity *context_lookup(Context *ctx, const char id[], Entity *tmp);
void context_let(Context *ctx, const char id[], const Entity *e);
void context_set(Context *ctx, const char id[], const Entity *e);

Context *task_get_context_for_index(Task *task, uint32_t index);

Object *wrap_function_in_ref(const Function *f, Object *obj, Heap *heap,
                             Context *ctx);
#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_PROCESS_CONTEXT_H_ */