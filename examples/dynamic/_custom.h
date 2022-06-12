// _custom.h
//
// Created on: Nov 27, 2021
//     Author: Jeff Manzione

#ifndef EXAMPLE_CUSTOM_H_
#define EXAMPLE_CUSTOM_H_

#include "entity/object.h"
#include "vm/module_manager.h"

#ifdef OS_WINDOWS
__declspec(dllexport)
#endif
    void _init_custom(ModuleManager *mm, Module *custom);

#endif /* EXAMPLE_CUSTOM_H_ */
