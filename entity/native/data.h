// data.h
//
// Created on: July 7, 2023
//     Author: Jeff Manzione

#ifndef ENTITY_NATIVE_DATA_H_
#define ENTITY_NATIVE_DATA_H_

#include "entity/object.h"
#include "vm/module_manager.h"

void data_add_native(ModuleManager *mm, Module *data);

#endif /* ENTITY_NATIVE_DATA_H_ */