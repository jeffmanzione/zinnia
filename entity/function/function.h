// func.h
//
// Created on: May 31, 2020
//     Author: Jeff Manzione

#ifndef OBJECT_FUNCTION_FUNCTION_H_
#define OBJECT_FUNCTION_FUNCTION_H_

#include <stdint.h>

#include "entity/object.h"

void function_init(Function *f, const char name[], const Module *module,
                   uint32_t ins_pos);
void function_finalize(Function *f);

#endif /* OBJECT_FUNCTION_FUNCTION_H_ */