// dynamic.h
//
// Created on: Nov 12, 2021
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_DYNAMIC_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_DYNAMIC_H_

#include "zinnia/entity/object.h"
#include "zinnia/vm/module_manager.h"

void dynamic_add_native(ModuleManager *mm, Module *dynamic);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_DYNAMIC_H_ */