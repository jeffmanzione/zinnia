// _custom.h
//
// Created on: Nov 27, 2021
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_EXAMPLE_CUSTOM_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_EXAMPLE_CUSTOM_H_

#include "zinnia/entity/native/native_hdrs.h"

#ifdef OS_WINDOWS
__declspec(dllexport)
#endif
    void _init_custom(ModuleManager *mm, Module *custom);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_EXAMPLE_CUSTOM_H_ */
