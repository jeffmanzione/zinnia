// math.c
//
// Created on: Nov 29, 2020
//     Author: Jeff Manzione

#include "zinnia/entity/native/math.h"

#include <math.h>

#include "language-tools/intern.h"
#include "zinnia/entity/class/classes_def.h"
#include "zinnia/entity/entity.h"
#include "zinnia/entity/native/error.h"
#include "zinnia/entity/native/native.h"
#include "zinnia/entity/native/native_helpers.h"
#include "zinnia/entity/object.h"
#include "zinnia/entity/primitive.h"
#include "zinnia/entity/tuple/tuple.h"
#include "zinnia/util/error.h"
#include "zinnia/vm/intern.h"


#define SingleFloatFn(name, val_var, body)                                     \
  Entity name##_(Task *task, Context *ctx, Object *obj, Entity *args) {        \
    if (NULL == args || PRIMITIVE != args->type) {                             \
      return raise_error(task, ctx,                                            \
                         #name "() takes exactly 1 primitive type argument."); \
    }                                                                          \
    const double val_var = float_of(&args->pri);                               \
    body;                                                                      \
  }

#define SingleFloatToCFn(name, cname) \
  SingleFloatFn(name, r, { return entity_float(cname(r)); });

double log_special_(double base, double num) {
  return log10(num) / log10(base);
}

Entity log_(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (IS_TUPLE(args)) {
    EXTRACT_TUPLE_ARGS(tuple, args, 2, task, ctx);
    EXTRACT_FLOAT_AT_INDEX_OR_THROW(const double num, tuple, 0);
    EXTRACT_FLOAT_AT_INDEX_OR_THROW(const double base, tuple, 1);
    return entity_float(log_special_(base, num));
  }
  if (args->type != PRIMITIVE) {
    return raise_error(task, ctx, "Cannot perform _log_ on a non-value.");
  }
  return entity_float(log(float_of(&args->pri)));
}

Entity pow_(Task *task, Context *ctx, Object *obj, Entity *args) {
  EXTRACT_TUPLE_ARGS(t, args, 2, task, ctx);
  EXTRACT_FLOAT_AT_INDEX_OR_THROW(const double num, t, 0);
  EXTRACT_FLOAT_AT_INDEX_OR_THROW(const double power, t, 1);

  return entity_float(pow(num, power));
}

Entity mod_(Task *task, Context *ctx, Object *obj, Entity *args) {
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
  native_function(math, global_intern("__log"), log_);
  native_function(math, global_intern("pow"), pow_);
  native_function(math, global_intern("sin"), sin_);
  native_function(math, global_intern("cos"), cos_);
  native_function(math, global_intern("tan"), tan_);
  native_function(math, global_intern("asin"), asin_);
  native_function(math, global_intern("acos"), acos_);
  native_function(math, global_intern("atan"), atan_);
  native_function(math, global_intern("exp"), exp_);
  native_function(math, global_intern("abs"), abs_);
  native_function(math, global_intern("ceil"), ceil_);
  native_function(math, global_intern("floor"), floor_);
  native_function(math, global_intern("mod"), mod_);
}