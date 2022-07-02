// math.c
//
// Created on: Nov 29, 2020
//     Author: Jeff Manzione

#include "entity/native/math.h"

#include <math.h>

#include "alloc/arena/intern.h"
#include "debug/debug.h"
#include "entity/class/classes_def.h"
#include "entity/entity.h"
#include "entity/native/error.h"
#include "entity/native/native.h"
#include "entity/object.h"
#include "entity/primitive.h"
#include "entity/tuple/tuple.h"
#include "vm/intern.h"

#define SingleFloatFn(name, val_var, body)                                     \
  Entity _##name(Task *task, Context *ctx, Object *obj, Entity *args) {        \
    if (NULL == args || PRIMITIVE != args->type) {                             \
      return raise_error(task, ctx,                                            \
                         #name "() takes exactly 1 primitive type argument."); \
    }                                                                          \
    const double val_var = float_of(&args->pri);                               \
    body;                                                                      \
  }

#define SingleFloatToCFn(name, cname)                                          \
  SingleFloatFn(name, r, { return entity_float(cname(r)); });

double _log_special(double base, double num) {
  return log10(num) / log10(base);
}

Entity _log_tuple(Task *task, Context *ctx, Tuple *tuple) {
  if (tuple_size(tuple) != 2) {
    return raise_error(task, ctx, "Invalid tuple arg to __log.");
  }
  const Entity *num = tuple_get(tuple, 0);
  const Entity *base = tuple_get(tuple, 1);
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

Entity _mod(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (NULL == args || OBJECT != args->type ||
      Class_Tuple != args->obj->_class) {
    return raise_error(task, ctx, "mod expects multiple arguments.");
  }
  Tuple *t = (Tuple *)args->obj->_internal_obj;
  if (tuple_size(t) != 2) {
    return raise_error(task, ctx, "mod expects exactly 2 arguments.");
  }
  const Entity *numerator = tuple_get(t, 0);
  const Entity *denominator = tuple_get(t, 1);

  if (numerator->type != PRIMITIVE) {
    return raise_error(task, ctx, "Cannot perform mod with non-numeric input.");
  }
  if (denominator->type != PRIMITIVE) {
    return raise_error(task, ctx, "Cannot perform mod with non-numeric power.");
  }
  return entity_float(
      fmod(float_of(&numerator->pri), float_of(&denominator->pri)));
}

SingleFloatToCFn(sin, sin);
SingleFloatToCFn(cos, cos);
SingleFloatToCFn(tan, tan);
SingleFloatToCFn(asin, asin);
SingleFloatToCFn(acos, acos);
SingleFloatToCFn(atan, atan);
SingleFloatToCFn(exp, exp);
SingleFloatToCFn(abs, fabs);
SingleFloatToCFn(ceil, ceil);
SingleFloatToCFn(floor, floor);

void math_add_native(ModuleManager *mm, Module *math) {
  native_function(math, intern("__log"), _log);
  native_function(math, intern("pow"), _pow);
  native_function(math, intern("sin"), _sin);
  native_function(math, intern("cos"), _cos);
  native_function(math, intern("tan"), _tan);
  native_function(math, intern("asin"), _asin);
  native_function(math, intern("acos"), _acos);
  native_function(math, intern("atan"), _atan);
  native_function(math, intern("exp"), _exp);
  native_function(math, intern("abs"), _abs);
  native_function(math, intern("ceil"), _ceil);
  native_function(math, intern("floor"), _floor);
  native_function(math, intern("mod"), _mod);
}