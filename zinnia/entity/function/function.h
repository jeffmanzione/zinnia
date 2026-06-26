// func.h
//
// Created on: May 31, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_OBJECT_FUNCTION_FUNCTION_H_
#define COM_GITHUB_JEFFMANZIONE_OBJECT_FUNCTION_FUNCTION_H_

#include <stdint.h>
#include <stdio.h>

#include "zinnia/entity/object.h"
#include "zinnia/heap/heap.h"

bool is_anon(const char name[]);

void function_init(Function *f, const char name[], const Module *module,
                   uint32_t ins_pos, bool is_anon, bool is_const,
                   bool is_async);
void function_finalize(Function *f);

void function_ref_create__(Object *obj);
void function_ref_init__(Object *fn_ref_obj, Object *obj, const Function *func,
                         void *parent_context, Heap *heap);
void function_ref_delete__(Object *obj);
void function_ref_print__(const Object *obj, FILE *out);

Object *function_ref_get_object(Object *fn_ref_obj);
const Function *function_ref_get_func(Object *fn_ref_obj);
void *function_ref_get_parent_context(Object *fn_ref_obj);

void function_ref_copy(EntityCopier *copier, const Object *src_obj,
                       Object *target_obj);

#endif /* COM_GITHUB_JEFFMANZIONE_OBJECT_FUNCTION_FUNCTION_H_ */