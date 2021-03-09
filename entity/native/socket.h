// socket.h
//
// Created on: Jan 27, 2019
//     Author: Jeff Manzione

#ifndef ENTITY_NATIVE_SOCKET_H_
#define ENTITY_NATIVE_SOCKET_H_

#include "entity/object.h"
#include "vm/module_manager.h"

void socket_add_native(ModuleManager *mm, Module *process);

#endif /* ENTITY_NATIVE_SOCKET_H_ */