// error.h
//
// Created on: Sept 15, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_ERROR_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_ERROR_H_

#include <stdarg.h>

#include "zinnia/entity/object.h"
#include "zinnia/vm/module_manager.h"
#include "zinnia/vm/process/processes.h"

extern Class *Class_StackLine;

Object *error_new(Task *task, Context *ctx, Object *error_msg);
void error_add_native(ModuleManager *mm, Module *error);

uint32_t stackline_linenum(Object *stackline);
Module *stackline_module(Object *stackline);

Entity raise_error(Task *task, Context *context, const char fmt[], ...);
Entity raise_error_with_object(Task *task, Context *context, Object *err);

Entity native_background_raise_error(Task *task, Context *context,
                                     const char fmt[], ...);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_ERROR_H_ */