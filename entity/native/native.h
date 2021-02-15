// native.h
//
// Created on: Aug 23, 2020
//     Author: Jeff Manzione

#ifndef ENTITY_NATIVE_NATIVE_H_
#define ENTITY_NATIVE_NATIVE_H_

#include "entity/class/class.h"
#include "entity/entity.h"
#include "entity/function/function.h"
#include "entity/object.h"
#include "vm/process/processes.h"

#define IS_CLASS(e, class)                                                     \
  (NULL != (e) && OBJECT == (e)->type && (class) == (e)->obj->_class)
#define IS_NONE(e) (NULL == (e) || NONE == (e)->type)
#define IS_OBJECT(e) (NULL != (e) && OBJECT == (e)->type)
#define IS_PRIMITIVE(e) (NULL != (e) && PRIMITIE == (e)->type)
#define IS_CHAR(e) (IS_PRIMITIVE(e) && CHAR == ptype(&(e)->pri))
#define IS_INT(e) (IS_PRIMITIVE(e) && INT == ptype(&(e)->pri))
#define IS_FLOAT(e) (IS_PRIMITIVE(e) && FLOAT == ptype(&(e)->pri))

#define IS_TUPLE(e)                                                            \
  ((NULL != e) && (OBJECT == e->type) && (Class_Tuple == e->obj->_class))

typedef Entity (*NativeFn)(Task *, Context *, Object *obj, Entity *args);

Function *native_method(Class *class, const char *name, NativeFn native_fn);
Function *native_background_method(Class *class, const char *name,
                                   NativeFn native_fn);
Function *native_function(Module *module, const char *name, NativeFn native_fn);
Class *native_class(Module *module, const char name[], ObjInitFn init_fn,
                    ObjDelFn del_fn);

#endif /* ENTITY_NATIVE_NATIVE_H_ */