// func.h
//
// Created on: May 31, 2020
//     Author: Jeff Manzione

#ifndef OBJECT_FUNCTION_FUNCTION_H_
#define OBJECT_FUNCTION_FUNCTION_H_

#include <stdint.h>
#include <stdio.h>

#include "entity/object.h"

void function_init(Function *f, const char name[], const Module *module,
                   uint32_t ins_pos);
void function_finalize(Function *f);

void __function_ref_create(Object *obj);
void __function_ref_init(Object *fn_ref_obj, Object *obj, Function *func);
void __function_ref_delete(Object *obj);
void __function_ref_print(const Object *obj, FILE *out);

Object *function_ref_get_object(Object *fn_ref_obj);
Function *function_ref_get_func(Object *fn_ref_obj);

#endif /* OBJECT_FUNCTION_FUNCTION_H_ */