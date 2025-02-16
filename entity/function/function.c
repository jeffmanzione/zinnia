// func.c
//
// Created on: May 31, 2020
//     Author: Jeff Manzione

#include "entity/function/function.h"

#include <stdio.h>
#include <string.h>

#include "alloc/alloc.h"
#include "debug/debug.h"
#include "entity/object.h"

typedef struct {
  Object *obj;
  const Function *func;
  void *parent_context; // To avoid circular dependency.
} _FunctionRef;

bool is_anon(const char name[]) { return 0 == strncmp("$anon_", name, 6); }

void function_init(Function *f, const char name[], const Module *module,
                   uint32_t ins_pos, bool is_anon, bool is_const,
                   bool is_async) {
  ASSERT(NOT_NULL(f), NOT_NULL(name));
  f->_name = name;
  f->_module = module;
  f->_parent_class = NULL;
  f->_ins_pos = ins_pos;
  f->_is_native = false;
  f->_is_anon = is_anon;
  f->_is_const = is_const;
  f->_is_async = is_async;
  f->_is_background = false;
  f->_reflection = NULL;
}

void function_finalize(Function *f) { ASSERT(NOT_NULL(f)); }

void __function_ref_create(Object *obj) { obj->_internal_obj = NULL; }

void __function_ref_init(Object *fn_ref_obj, Object *obj, const Function *func,
                         void *parent_context) {
  _FunctionRef *func_ref =
      (_FunctionRef *)(fn_ref_obj->_internal_obj = ALLOC2(_FunctionRef));
  func_ref->obj = obj;
  func_ref->func = func;
  func_ref->parent_context = parent_context;
}

void __function_ref_delete(Object *obj) {
  if (NULL == obj->_internal_obj) {
    return;
  }
  DEALLOC(obj->_internal_obj);
}

void __function_ref_print(const Object *obj, FILE *out) {
  fprintf(out, "Instance of builtin.FunctionRef");
}

Object *function_ref_get_object(Object *fn_ref_obj) {
  _FunctionRef *func_ref = (_FunctionRef *)fn_ref_obj->_internal_obj;
  return func_ref->obj;
}

const Function *function_ref_get_func(Object *fn_ref_obj) {
  _FunctionRef *func_ref = (_FunctionRef *)fn_ref_obj->_internal_obj;
  return func_ref->func;
}

void *function_ref_get_parent_context(Object *fn_ref_obj) {
  _FunctionRef *func_ref = (_FunctionRef *)fn_ref_obj->_internal_obj;
  return func_ref->parent_context;
}

void function_ref_copy(EntityCopier *copier, const Object *src_obj,
                       Object *target_obj) {
  _FunctionRef *func_ref = (_FunctionRef *)src_obj->_internal_obj;
  Entity e = entity_object(func_ref->obj);
  Entity cpy_obj = entitycopier_copy(copier, &e);
  // TODO: Deep copy parent context?
  __function_ref_init(target_obj, cpy_obj.obj, func_ref->func,
                      func_ref->parent_context);
}