// math.c
//
// Created on: Nov 29, 2020
//     Author: Jeff Manzione

#include "examples/dynamic/_custom.h"

#include <math.h>
#include <stdio.h>

void sin_impl(FunctionContext *fn_ctx) {
  const Entity *args = FunctionContext_args(fn_ctx);
  if (NULL == args || PRIMITIVE != args->type) {
    FunctionContext_raise_error(
        fn_ctx, "sin() takes exactly 1 primitive type argument.");
    return;
  }
  const double r = float_of(&args->pri);
  *FunctionContext_mutable_retval(fn_ctx) = entity_float(sin(r));
}

void cos_impl(FunctionContext *fn_ctx) {
  const Entity *args = FunctionContext_args(fn_ctx);
  if (NULL == args || PRIMITIVE != args->type) {
    FunctionContext_raise_error(
        fn_ctx, "cos() takes exactly 1 primitive type argument.");
    return;
  }
  const double r = float_of(&args->pri);
  *FunctionContext_mutable_retval(fn_ctx) = entity_float(cos(r));
}

void tan_impl(FunctionContext *fn_ctx) {
  const Entity *args = FunctionContext_args(fn_ctx);
  if (NULL == args || PRIMITIVE != args->type) {
    FunctionContext_raise_error(
        fn_ctx, "tan() takes exactly 1 primitive type argument.");
    return;
  }
  const double r = float_of(&args->pri);
  *FunctionContext_mutable_retval(fn_ctx) = entity_float(tan(r));
}

void init_custom(ModuleBuilder *builder) {
  ModuleBuilder_verify_signature(builder, init_custom);
  ModuleBuilder_add_function(builder, "sin", sin_impl);
  ModuleBuilder_add_function(builder, "cos", cos_impl);
  ModuleBuilder_add_function(builder, "tan", tan_impl);
}