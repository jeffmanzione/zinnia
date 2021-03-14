// io.h
//
// Created on: Sept 02, 2020
//     Author: Jeff Manzione

#ifndef ENTITY_NATIVE_IO_H_
#define ENTITY_NATIVE_IO_H_

#include "entity/object.h"
#include "vm/module_manager.h"

void io_add_native(ModuleManager *mm, Module *io);

#endif /* ENTITY_NATIVE_IO_H_ */