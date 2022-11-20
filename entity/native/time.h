// time.h
//
// Created on: Nov 12, 2022
//     Author: Jeff Manzione

#ifndef ENTITY_NATIVE_TIME_H_
#define ENTITY_NATIVE_TIME_H_

#include "entity/object.h"
#include "vm/module_manager.h"

void time_add_native(ModuleManager *mm, Module *math);

#endif /* ENTITY_NATIVE_TIME_H_ */