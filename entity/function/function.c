// func.c
//
// Created on: May 31, 2020
//     Author: Jeff Manzione

#include "entity/function/function.h"

#include <stdio.h>

#include "alloc/alloc.h"
#include "debug/debug.h"
#include "entity/object.h"

typedef struct {
  Object *obj;
  const Function *func;
} _FunctionRef;

inline void function_init(Function *f, const char name[], const Module *module,
                          uint32_t ins_pos) {
  ASSERT(NOT_NULL(f), NOT_NULL(name));
  f->_name = name;
  f->_module = module;
  f->_parent_class = NULL;
  f->_ins_pos = ins_pos;
  f->_is_native = false;
  f->_reflection = NULL;
}

inline void function_finalize(Function *f) { ASSERT(NOT_NULL(f)); }

inline void __function_ref_create(Object *obj) { obj->_internal_obj = NULL; }

inline void __function_ref_init(Object *fn_ref_obj, Object *obj,
                                const Function *func) {
  _FunctionRef *func_ref =
      (_FunctionRef *)(fn_ref_obj->_internal_obj = ALLOC2(_FunctionRef));
  func_ref->obj = obj;
  func_ref->func = func;
}

inline void __function_ref_delete(Object *obj) {
  if (NULL == obj->_internal_obj) {
    return;
  }
  DEALLOC(obj->_internal_obj);
}

void __function_ref_print(const Object *obj, FILE *out) {
  fprintf(out, "Instance of builtin.FunctionRef");
}

inline Object *function_ref_get_object(Object *fn_ref_obj) {
  _FunctionRef *func_ref = (_FunctionRef *)fn_ref_obj->_internal_obj;
  return func_ref->obj;
}

inline const Function *function_ref_get_func(Object *fn_ref_obj) {
  _FunctionRef *func_ref = (_FunctionRef *)fn_ref_obj->_internal_obj;
  return func_ref->func;
}