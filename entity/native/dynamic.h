// dynamic.h
//
// Created on: Nov 12, 2021
//     Author: Jeff Manzione

#ifndef ENTITY_NATIVE_DYNAMIC_H_
#define ENTITY_NATIVE_DYNAMIC_H_

#include "entity/object.h"
#include "vm/module_manager.h"

void dynamic_add_native(ModuleManager *mm, Module *dynamic);

#endif /* ENTITY_NATIVE_DYNAMIC_H_ */