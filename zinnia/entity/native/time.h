// time.h
//
// Created on: Nov 12, 2022
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_TIME_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_TIME_H_

#include "zinnia/entity/object.h"
#include "zinnia/vm/module_manager.h"

void time_add_native(ModuleManager *mm, Module *math);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_TIME_H_ */