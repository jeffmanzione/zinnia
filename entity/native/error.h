// error.h
//
// Created on: Sept 15, 2020
//     Author: Jeff Manzione

#ifndef ENTITY_NATIVE_ERROR_H_
#define ENTITY_NATIVE_ERROR_H_

#include "entity/object.h"
#include "vm/process/processes.h"

extern Class *Class_StackLine;

Object *error_new(Task *task, Context *ctx, Object *error_msg);
void error_add_native(Module *error);

uint32_t stackline_linenum(Object *stackline);
Module *stackline_module(Object *stackline);

#endif /* ENTITY_NATIVE_ERROR_H_ */