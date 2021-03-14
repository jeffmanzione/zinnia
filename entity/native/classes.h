// classes.h
//
// Created on: Jan 1, 2021
//     Author: Jeff Manzione

#ifndef ENTITY_NATIVE_CLASSES_H_
#define ENTITY_NATIVE_CLASSES_H_

#include "entity/object.h"
#include "vm/module_manager.h"

void classes_add_native(ModuleManager *mm, Module *classes);

#endif /* ENTITY_NATIVE_CLASSES_H_ */
