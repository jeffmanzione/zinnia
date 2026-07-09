// _custom.h
//
// Created on: Nov 27, 2021
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_EXAMPLE_CUSTOM_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_EXAMPLE_CUSTOM_H_

#include "zinnia/native.h"

NATIVE_FN void init_custom(ModuleBuilder *builder);

NATIVE_FN void sin_impl(FunctionContext *fn_ctx);

NATIVE_FN void cos_impl(FunctionContext *fn_ctx);

NATIVE_FN void tan_impl(FunctionContext *fn_ctx);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_EXAMPLE_CUSTOM_H_ */
