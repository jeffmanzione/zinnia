// error.h
//
// Created on: Sept 15, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_ERROR_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_ERROR_H_

#include <stdarg.h>

#include "zinnia/entity/object.h"
#include "zinnia/vm/isolate/isolates.h"
#include "zinnia/vm/module_manager.h"

extern Class *Class_StackLine;

Object *error_new(Frame *frame, Scope *ctx, Object *error_msg);
void error_add_native(ModuleManager *mm, Module *error);

uint32_t stackline_linenum(Object *stackline);
Module *stackline_module(Object *stackline);

Entity raise_error(Frame *frame, Scope *scope, const char fmt[], ...);
Entity raise_errorv(Frame *frame, Scope *scope, const char fmt[], va_list args);
Entity raise_error_with_object(Frame *frame, Scope *scope, Object *err);

Entity native_blocking_raise_error(Frame *frame, Scope *scope, const char fmt[],
                                   ...);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_ERROR_H_ */