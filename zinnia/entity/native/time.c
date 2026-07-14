// time.c
//
// Created on: Nov 12, 2022
//     Author: Jeff Manzione

#include "zinnia/entity/native/time.h"

#include "zinnia/util/time.h"

#define USEC_PER_SEC 1000 * 1000;

void now_usec_(FunctionContext *fn_ctx) {
  *FunctionContext_mutable_retval(fn_ctx) =
      entity_int(current_usec_since_epoch());
}

void now_timestamp_(FunctionContext *fn_ctx) {
  Timestamp ts = current_local_timestamp();
  Object *t_obj = FunctionContext_create_tuple(
      fn_ctx, 7, entity_int(ts.year), entity_int(ts.month),
      entity_int(ts.day_of_month), entity_int(ts.hour), entity_int(ts.minute),
      entity_int(ts.second), entity_int(ts.millisecond));
  FunctionContext_set_retval_obj(fn_ctx, t_obj);
}

void timestamp_to_micros_(FunctionContext *fn_ctx) {
  Timestamp ts;
  EXTRACT_TUPLE_ARGS2(tuple, fn_ctx, 7);
  EXTRACT_INT_AT_INDEX_OR_THROW2(ts.year, fn_ctx, tuple, 0);
  EXTRACT_INT_AT_INDEX_OR_THROW2(ts.month, fn_ctx, tuple, 1);
  EXTRACT_INT_AT_INDEX_OR_THROW2(ts.day_of_month, fn_ctx, tuple, 2);
  EXTRACT_INT_AT_INDEX_OR_THROW2(ts.hour, fn_ctx, tuple, 3);
  EXTRACT_INT_AT_INDEX_OR_THROW2(ts.minute, fn_ctx, tuple, 4);
  EXTRACT_INT_AT_INDEX_OR_THROW2(ts.second, fn_ctx, tuple, 5);
  EXTRACT_INT_AT_INDEX_OR_THROW2(ts.millisecond, fn_ctx, tuple, 6);
  *FunctionContext_mutable_retval(fn_ctx) =
      entity_int(timestamp_to_micros(&ts));
}

void micros_to_timestamp_(FunctionContext *fn_ctx) {
  const Entity *args = FunctionContext_args(fn_ctx);
  if (!IS_INT(args)) {
    FunctionContext_raise_error(fn_ctx, "Expected an int.");
    return;
  }
  int64_t micros_since_epoch = pint(&args->pri);
  Timestamp ts = micros_to_timestamp(micros_since_epoch);

  Object *t_obj = FunctionContext_create_tuple(
      fn_ctx, 7, entity_int(ts.year), entity_int(ts.month),
      entity_int(ts.day_of_month), entity_int(ts.hour), entity_int(ts.minute),
      entity_int(ts.second), entity_int(ts.millisecond));
  FunctionContext_set_retval_obj(fn_ctx, t_obj);
}

void time_add_native(ModuleBuilder *builder) {
  ModuleBuilder_verify_signature(builder, time_add_native);
  ModuleBuilder_add_function(builder, "__now_usec", now_usec_);
  ModuleBuilder_add_function(builder, "__now_timestamp", now_timestamp_);
  ModuleBuilder_add_function(builder, "__timestamp_to_micros",
                             timestamp_to_micros_);
  ModuleBuilder_add_function(builder, "__micros_to_timestamp",
                             micros_to_timestamp_);
}