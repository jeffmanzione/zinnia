// func.h
//
// Created on: May 31, 2020
//     Author: Jeff Manzione

#ifndef OBJECT_FUNCTION_FUNCTION_H_
#define OBJECT_FUNCTION_FUNCTION_H_

#include <stdint.h>
#include <stdio.h>

#include "entity/object.h"
#include "heap/heap.h"

bool is_anon(const char name[]);

void function_init(Function *f, const char name[], const Module *module,
                   uint32_t ins_pos, bool is_anon, bool is_const,
                   bool is_async);
void function_finalize(Function *f);

void __function_ref_create(Object *obj);
void __function_ref_init(Object *fn_ref_obj, Object *obj, const Function *func,
                         void *parent_context);
void __function_ref_delete(Object *obj);
void __function_ref_print(const Object *obj, FILE *out);

Object *function_ref_get_object(Object *fn_ref_obj);
const Function *function_ref_get_func(Object *fn_ref_obj);
void *function_ref_get_parent_context(Object *fn_ref_obj);

void function_ref_copy(Heap *heap, Map *cpy_map, Object *target_obj,
                       Object *src_obj);

#endif /* OBJECT_FUNCTION_FUNCTION_H_ */