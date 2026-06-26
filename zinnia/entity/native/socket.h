// socket.h
//
// Created on: Jan 27, 2019
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_SOCKET_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_SOCKET_H_

#include "zinnia/entity/object.h"
#include "zinnia/vm/module_manager.h"

void socket_add_native(ModuleManager *mm, Module *process);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_SOCKET_H_ */