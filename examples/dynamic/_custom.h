// _custom.h
//
// Created on: Nov 27, 2021
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_EXAMPLE_CUSTOM_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_EXAMPLE_CUSTOM_H_

#include "zinnia/native.h"

#ifdef OS_WINDOWS
__declspec(dllexport)
#endif
    void init_custom(NativeModuleBuilder *builder);

#ifdef OS_WINDOWS
__declspec(dllexport)
#endif
    void sin_impl(NativeFunctionContext *fn_ctx);

#ifdef OS_WINDOWS
__declspec(dllexport)
#endif
    void cos_impl(NativeFunctionContext *fn_ctx);

#ifdef OS_WINDOWS
__declspec(dllexport)
#endif
    void tan_impl(NativeFunctionContext *fn_ctx);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_EXAMPLE_CUSTOM_H_ */
