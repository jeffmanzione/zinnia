// scope.h
//
// Created on: Jun 13, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_ISOLATE_SCOPE_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_ISOLATE_SCOPE_H_

#include "zinnia/entity/module/module.h"
#include "zinnia/entity/object.h"
#include "zinnia/program/instruction.h"
#include "zinnia/vm/isolate/isolates.h"

void scope_init(Scope *ctx, Object *self, Module *module,
                uint32_t instruction_pos);
void scope_finalize(Scope *ctx);
Object *scope_self(Scope *ctx);
Module *scope_module(Scope *ctx);
const Instruction *scope_ins(Scope *ctx);
void scope_set_function(Scope *ctx, const Function *func);
Value *scope_lookup(Scope *ctx, const char id[], Value *tmp);
void scope_let(Scope *ctx, const char id[], const Value *e);
void scope_set(Scope *ctx, const char id[], const Value *e);

Scope *frame_get_scope_for_index(Frame *frame, uint32_t index);

Object *wrap_function_in_ref(const Function *f, Object *obj, Heap *heap,
                             Scope *ctx);
#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_ISOLATE_SCOPE_H_ */