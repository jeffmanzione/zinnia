// math.c
//
// Created on: Nov 29, 2020
//     Author: Jeff Manzione

#include "zinnia/entity/native/math.h"

#include <math.h>

#define SingleFloatFn(name, val_var, body)                                     \
  Value name##_(Frame *frame, Scope *ctx, Object *obj, Value *args) {          \
    if (NULL == args || PRIMITIVE != args->type) {                             \
      return raise_error(frame, ctx,                                           \
                         #name "() takes exactly 1 primitive type argument."); \
    }                                                                          \
    const double val_var = float_of(&args->pri);                               \
    body;                                                                      \
  }

#define SingleFloatToCFn(name, cname) \
  SingleFloatFn(name, r, { return value_float(cname(r)); });

double log_special_(double base, double num) {
  return log10(num) / log10(base);
}

Value log_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  if (IS_TUPLE(args)) {
    EXTRACT_TUPLE_ARGS(tuple, args, 2, frame, ctx);
    EXTRACT_FLOAT_AT_INDEX_OR_THROW(const double num, tuple, 0);
    EXTRACT_FLOAT_AT_INDEX_OR_THROW(const double base, tuple, 1);
    return value_float(log_special_(base, num));
  }
  if (args->type != PRIMITIVE) {
    return raise_error(frame, ctx, "Cannot perform _log_ on a non-value.");
  }
  return value_float(log(float_of(&args->pri)));
}

Value pow_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  EXTRACT_TUPLE_ARGS(t, args, 2, frame, ctx);
  EXTRACT_FLOAT_AT_INDEX_OR_THROW(const double num, t, 0);
  EXTRACT_FLOAT_AT_INDEX_OR_THROW(const double power, t, 1);

  return value_float(pow(num, power));
}

Value mod_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  EXTRACT_TUPLE_ARGS(t, args, 2, frame, ctx);
  EXTRACT_FLOAT_AT_INDEX_OR_THROW(const double numerator, t, 0);
  EXTRACT_FLOAT_AT_INDEX_OR_THROW(const double denominator, t, 1);

  return value_float(fmod(numerator, denominator));
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