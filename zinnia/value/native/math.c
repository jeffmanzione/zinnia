// math.c
//
// Created on: Nov 29, 2020
//     Author: Jeff Manzione

#include "zinnia/value/native/math.h"

#include <math.h>

// Like EXTRACT_FLOAT_AT_INDEX_OR_THROW, but also accepts Ints.
#define EXTRACT_NUMBER_AT_INDEX_OR_THROW(var, tuple, index)                    \
  if (!IS_FLOAT(tuple_get(tuple, index)) &&                                    \
      !IS_INT(tuple_get(tuple, index))) {                                      \
    return raise_error(frame, ctx,                                             \
                       "Expected argument at index %d to be a number", index); \
  }                                                                            \
  var = float_of(&tuple_get(tuple, index)->pri);

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
    EXTRACT_NUMBER_AT_INDEX_OR_THROW(const double num, tuple, 0);
    EXTRACT_NUMBER_AT_INDEX_OR_THROW(const double base, tuple, 1);
    return value_float(log_special_(base, num));
  }
  if (args->type != PRIMITIVE) {
    return raise_error(frame, ctx, "Cannot perform _log_ on a non-value.");
  }
  return value_float(log(float_of(&args->pri)));
}

Value pow_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  EXTRACT_TUPLE_ARGS(t, args, 2, frame, ctx);
  EXTRACT_NUMBER_AT_INDEX_OR_THROW(const double num, t, 0);
  EXTRACT_NUMBER_AT_INDEX_OR_THROW(const double power, t, 1);

  return value_float(pow(num, power));
}

Value mod_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  EXTRACT_TUPLE_ARGS(t, args, 2, frame, ctx);
  if (IS_INT(tuple_get(t, 0)) && IS_INT(tuple_get(t, 1))) {
    const int64_t denominator = int_of(&tuple_get(t, 1)->pri);
    if (0 == denominator) {
      return raise_error(frame, ctx, "mod() by zero.");
    }
    return value_int(int_of(&tuple_get(t, 0)->pri) % denominator);
  }
  EXTRACT_NUMBER_AT_INDEX_OR_THROW(const double numerator, t, 0);
  EXTRACT_NUMBER_AT_INDEX_OR_THROW(const double denominator, t, 1);

  return value_float(fmod(numerator, denominator));
}

SingleFloatToCFn(sin, sin);
SingleFloatToCFn(cos, cos);
SingleFloatToCFn(tan, tan);
SingleFloatToCFn(asin, asin);
SingleFloatToCFn(acos, acos);
SingleFloatToCFn(atan, atan);
SingleFloatToCFn(exp, exp);
Value abs_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  if (NULL != args && IS_INT(args)) {
    const int64_t val = int_of(&args->pri);
    return value_int(val < 0 ? -val : val);
  }
  if (NULL == args || PRIMITIVE != args->type) {
    return raise_error(frame, ctx,
                       "abs() takes exactly 1 primitive type argument.");
  }
  return value_float(fabs(float_of(&args->pri)));
}
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