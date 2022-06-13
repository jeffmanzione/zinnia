// math.c
//
// Created on: Nov 29, 2020
//     Author: Jeff Manzione

#include "examples/dynamic/_custom.h"

#include <math.h>

#include "alloc/arena/intern.h"
#include "debug/debug.h"
#include "entity/class/classes.h"
#include "entity/entity.h"
#include "entity/native/error.h"
#include "entity/native/native.h"
#include "entity/object.h"
#include "entity/primitive.h"
#include "entity/tuple/tuple.h"
#include "vm/intern.h"

#include "util/platform.h"

Entity _sin(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (NULL == args || PRIMITIVE != args->type) {
    return raise_error(task, ctx,
                       "sin() takes exactly 1 primitive type argument.");
  }
  const double r = float_of(&args->pri);
  return entity_float(sin(r));
}

Entity _cos(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (NULL == args || PRIMITIVE != args->type) {
    return raise_error(task, ctx,
                       "cos() takes exactly 1 primitive type argument.");
  }
  const double r = float_of(&args->pri);
  return entity_float(cos(r));
}

Entity _tan(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (NULL == args || PRIMITIVE != args->type) {
    return raise_error(task, ctx,
                       "tan() takes exactly 1 primitive type argument.");
  }
  const double r = float_of(&args->pri);
  return entity_float(tan(r));
}

void _init_custom(ModuleManager *mm, Module *custom) {
  native_function(custom, mm->intern("sin"), _sin);
  native_function(custom, mm->intern("cos"), _cos);
  native_function(custom, mm->intern("tan"), _tan);
}