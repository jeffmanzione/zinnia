// builtin.h
//
// Created on: Aug 29, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_BUILTIN_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_BUILTIN_H_

#include <stdint.h>

#include "zinnia/entity/object.h"
#include "zinnia/vm/module_manager.h"

typedef struct {
  int32_t start;
  int32_t end;
  int32_t inc;
} Range_;

void builtin_add_native(ModuleManager *mm, Module *builtin);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_BUILTIN_H_ */