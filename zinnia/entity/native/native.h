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

// Sets the frame of the blocking call running on this thread, or NULL once it
// returns. Objects the call allocates are kept alive as long as the frame, so
// the isolate's garbage collector cannot delete them before the result is
// passed back to the isolate.
void native_set_blocking_frame(Frame *frame);
// Keeps obj alive as long as the blocking call running on this thread, if any.
// Must hold isolate's heap lock.
void native_pin_blocking_object(Isolate *isolate, Object *obj);

// Allocate objects from a blocking call. They are pinned to it.
Object *native_blocking_new(Isolate *isolate, Class *class);
Object *native_blocking_string_new(Isolate *isolate, const char src[],
                                   size_t len);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_NATIVE_H_ */