// scope.h
//
// Created on: Jun 13, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_ISOLATE_SCOPE_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_ISOLATE_SCOPE_H_

#include "zinnia/program/instruction.h"
#include "zinnia/value/module/module.h"
#include "zinnia/value/object.h"
#include "zinnia/vm/isolate/isolates.h"

void scope_init(Scope *ctx, Object *self, Module *module,
                uint32_t instruction_pos);
void scope_finalize(Scope *ctx);
Object *scope_self(Scope *ctx);
Module *scope_module(Scope *ctx);
const Instruction *scope_ins(Scope *ctx);
// Sets the scope [ctx] is nested in, which it keeps alive.
void scope_set_previous(Scope *ctx, Scope *previous);
// Keeps [ctx] alive as long as [fn_ref], an anonymous function defined in it.
void scope_capture(Scope *ctx, Object *fn_ref, Heap *heap);
void scope_set_function(Scope *ctx, const Function *func);
Value *scope_lookup(Scope *ctx, const char id[], Value *tmp);
void scope_let(Scope *ctx, const char id[], const Value *e);
// Assigns [e] to the variable [id], wherever it is defined. Returns false
// without assigning if that variable is const.
bool scope_set(Scope *ctx, const char id[], const Value *e);
// Defines [id] in this scope as a const with the value [e]. Returns false
// without assigning if [id] is already a const in this scope.
bool scope_let_const(Scope *ctx, const char id[], const Value *e);
// Returns true if [id] resolves to a const variable from this scope.
bool scope_is_const(Scope *ctx, const char id[]);

Scope *frame_get_scope_for_index(Frame *frame, uint32_t index);

Object *wrap_function_in_ref(const Function *f, Object *obj, Heap *heap,
                             Scope *ctx);
#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_ISOLATE_SCOPE_H_ */