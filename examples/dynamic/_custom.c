// math.c
//
// Created on: Nov 29, 2020
//     Author: Jeff Manzione

#include "examples/dynamic/_custom.h"

#include <math.h>

Entity __sin_impl(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (NULL == args || PRIMITIVE != args->type) {
    return raise_error(task, ctx,
                       "sin() takes exactly 1 primitive type argument.");
  }
  const double r = float_of(&args->pri);
  return entity_float(sin(r));
}

Entity __cos_impl(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (NULL == args || PRIMITIVE != args->type) {
    return raise_error(task, ctx,
                       "cos() takes exactly 1 primitive type argument.");
  }
  const double r = float_of(&args->pri);
  return entity_float(cos(r));
}

Entity __tan_impl(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (NULL == args || PRIMITIVE != args->type) {
    return raise_error(task, ctx,
                       "tan() takes exactly 1 primitive type argument.");
  }
  const double r = float_of(&args->pri);
  return entity_float(tan(r));
}

void _init_custom(ModuleManager *mm, Module *custom) {
  verify_init_fn_signature(_init_custom);

  native_function(custom, mm->intern("sin"), __sin_impl);
  native_function(custom, mm->intern("cos"), __cos_impl);
  native_function(custom, mm->intern("tan"), __tan_impl);
}