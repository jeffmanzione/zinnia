// native.h
//
// Created on: Aug 23, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_NATIVE_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_NATIVE_H_

#include "zinnia/entity/class/class.h"
#include "zinnia/entity/function/function.h"
#include "zinnia/entity/object.h"
#include "zinnia/entity/value.h"
#include "zinnia/vm/isolate/isolates.h"

typedef Value (*NativeFn)(Frame *, Scope *, Object *obj, Value *args);

Function *native_method(Class *class, const char *name, NativeFn native_fn);
Function *native_blocking_method(Class *class, const char *name,
                                 NativeFn native_fn);
Function *native_function(Module *module, const char *name, NativeFn native_fn);
Function *native_blocking_function(Module *module, const char name[],
                                   NativeFn native_fn);
Class *native_class(Module *module, const char name[], ObjInitFn init_fn,
                    ObjDelFn del_fn);

Object *native_blocking_new(Isolate *isolate, Class *class);
Object *native_blocking_string_new(Isolate *isolate, const char src[],
                                   size_t len);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_NATIVE_H_ */