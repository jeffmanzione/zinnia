// io.h
//
// Created on: Sept 02, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_IO_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_IO_H_

#include "zinnia/entity/object.h"
#include "zinnia/vm/module_manager.h"

void io_add_native(ModuleManager *mm, Module *io);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_IO_H_ */