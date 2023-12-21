// builtin.h
//
// Created on: Aug 29, 2020
//     Author: Jeff Manzione

#ifndef ENTITY_NATIVE_BUILTIN_H_
#define ENTITY_NATIVE_BUILTIN_H_

#include "entity/object.h"
#include "vm/module_manager.h"

typedef struct {
  int32_t start;
  int32_t end;
  int32_t inc;
} _Range;

void builtin_add_native(ModuleManager *mm, Module *builtin);

#endif /* ENTITY_NATIVE_BUILTIN_H_ */