// math.c
//
// Created on: Nov 29, 2020
//     Author: Jeff Manzione

#include "entity/native/math.h"

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

double _log_special(double base, double num) {
  return log10(num) / log10(base);
}

Entity _log_tuple(Task *task, Context *ctx, Tuple *tuple) {
  if (tuple_size(tuple) != 2) {
    return raise_error(task, ctx, "Invalid tuple arg to __log.");
  }
  const Entity *base = tuple_get(tuple, 0);
  const Entity *num = tuple_get(tuple, 1);

  if (base->type != PRIMITIVE) {
    return raise_error(task, ctx,
                       "Cannot perform __log with non-numeric base.");
  }
  if (num->type != PRIMITIVE) {
    return raise_error(task, ctx,
                       "Cannot perform __log with non-numeric input.");
  }
  return entity_float(_log_special(float_of(&base->pri), float_of(&num->pri)));
}

Entity _log(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (NULL != args && OBJECT == args->type &&
      Class_Tuple == args->obj->_class) {
    return _log_tuple(task, ctx, (Tuple *)args->obj->_internal_obj);
  }
  if (args->type != PRIMITIVE) {
    return raise_error(task, ctx, "Cannot perform __log on a non-value.");
  }
  return entity_float(log(float_of(&args->pri)));
}

Entity _pow(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (NULL == args || OBJECT != args->type ||
      Class_Tuple != args->obj->_class) {
    return raise_error(task, ctx, "__pow expects multiple arguments.");
  }
  Tuple *t = (Tuple *)args->obj->_internal_obj;
  if (tuple_size(t) != 2) {
    return raise_error(task, ctx, "__pow expects exactly 2 arguments.");
  }
  const Entity *num = tuple_get(t, 0);
  const Entity *power = tuple_get(t, 1);

  if (num->type != PRIMITIVE) {
    return raise_error(task, ctx,
                       "Cannot perform __pow with non-numeric input.");
  }
  if (power->type != PRIMITIVE) {
    return raise_error(task, ctx,
                       "Cannot perform __pow with non-numeric power.");
  }
  return entity_float(pow(float_of(&num->pri), float_of(&num->pri)));
}

void math_add_native(Module *math) {
  native_function(math, intern("__log"), _log);
  native_function(math, intern("__pow"), _pow);
}