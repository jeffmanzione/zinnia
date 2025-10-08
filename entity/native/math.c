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
#include "entity/native/native_helpers.h"
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

#define SingleFloatToCFn(name, cname) \
  SingleFloatFn(name, r, { return entity_float(cname(r)); });

double _log_special(double base, double num) {
  return log10(num) / log10(base);
}

Entity _log(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (IS_TUPLE(args)) {
    EXTRACT_TUPLE_ARGS(tuple, args, 2, task, ctx);
    EXTRACT_FLOAT_AT_INDEX_OR_THROW(const double num, tuple, 0);
    EXTRACT_FLOAT_AT_INDEX_OR_THROW(const double base, tuple, 1);
    return entity_float(_log_special(base, num));
  }
  if (args->type != PRIMITIVE) {
    return raise_error(task, ctx, "Cannot perform __log on a non-value.");
  }
  return entity_float(log(float_of(&args->pri)));
}

Entity _pow(Task *task, Context *ctx, Object *obj, Entity *args) {
  EXTRACT_TUPLE_ARGS(t, args, 2, task, ctx);
  EXTRACT_FLOAT_AT_INDEX_OR_THROW(const double num, t, 0);
  EXTRACT_FLOAT_AT_INDEX_OR_THROW(const double power, t, 1);

  return entity_float(pow(num, power));
}

Entity _mod(Task *task, Context *ctx, Object *obj, Entity *args) {
  EXTRACT_TUPLE_ARGS(t, args, 2, task, ctx);
  EXTRACT_FLOAT_AT_INDEX_OR_THROW(const double numerator, t, 0);
  EXTRACT_FLOAT_AT_INDEX_OR_THROW(const double denominator, t, 1);

  return entity_float(fmod(numerator, denominator));
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