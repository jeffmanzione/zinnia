// context.c
//
// Created on: Jun 13, 2020
//     Author: Jeff Manzione

#include "vm/process/context.h"

#include "program/tape.h"
#include "vm/process/processes.h"

void context_init(Context *ctx, Object *self, Object *member_obj,
                  Module *module, uint32_t instruction_pos) {
  ctx->self = self;
  ctx->member_obj = member_obj;
  ctx->module = module;
  ctx->tape = module->_tape;
  ctx->is_function = false;
}
void context_finalize(Context *ctx) {}

inline const Instruction *context_ins(Context *ctx) {
  return tape_get(ctx->tape, ctx->ins);
}

inline Object *context_self(Context *ctx) { return ctx->self; }

inline Module *context_module(Context *ctx) { return ctx->module; }

Entity context_lookup(Context *ctx, const char id[]) { return entity_none(); }

void context_let(Context *ctx, const char id[], const Entity *e) {}

void context_set(Context *ctx, const char id[], const Entity *e) {}