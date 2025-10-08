// time.c
//
// Created on: Nov 12, 2022
//     Author: Jeff Manzione

#include "entity/native/time.h"

#include "alloc/arena/intern.h"
#include "entity/class/classes_def.h"
#include "entity/entity.h"
#include "entity/native/error.h"
#include "entity/native/native.h"
#include "entity/native/native_helpers.h"
#include "entity/object.h"
#include "entity/tuple/tuple.h"
#include "heap/heap.h"
#include "util/time.h"

#define USEC_PER_SEC 1000 * 1000;

Entity _now_usec(Task *task, Context *ctx, Object *obj, Entity *args) {
  return entity_int(current_usec_since_epoch());
}

Entity _timestamp_to_entity(Heap *heap, Timestamp *ts) {
  Entity year = entity_int(ts->year);
  Entity month = entity_int(ts->month);
  Entity day_of_month = entity_int(ts->day_of_month);
  Entity hour = entity_int(ts->hour);
  Entity minute = entity_int(ts->minute);
  Entity second = entity_int(ts->second);
  Entity millisecond = entity_int(ts->millisecond);
  return entity_object(tuple_create7(heap, &year, &month, &day_of_month, &hour,
                                     &minute, &second, &millisecond));
}

Entity _now_timestamp(Task *task, Context *ctx, Object *obj, Entity *args) {
  Timestamp ts = current_local_timestamp();
  return _timestamp_to_entity(task->parent_process->heap, &ts);
}

Entity _timestamp_to_micros(Task *task, Context *ctx, Object *obj,
                            Entity *args) {
  Timestamp ts;
  EXTRACT_TUPLE_ARGS(tuple, args, 7, task, ctx);
  EXTRACT_INT_AT_INDEX_OR_THROW(ts.year, tuple, 0);
  EXTRACT_INT_AT_INDEX_OR_THROW(ts.month, tuple, 1);
  EXTRACT_INT_AT_INDEX_OR_THROW(ts.day_of_month, tuple, 2);
  EXTRACT_INT_AT_INDEX_OR_THROW(ts.hour, tuple, 3);
  EXTRACT_INT_AT_INDEX_OR_THROW(ts.minute, tuple, 4);
  EXTRACT_INT_AT_INDEX_OR_THROW(ts.second, tuple, 5);
  EXTRACT_INT_AT_INDEX_OR_THROW(ts.millisecond, tuple, 6);

  return entity_int(timestamp_to_micros(&ts));
}

Entity _micros_to_timestamp(Task *task, Context *ctx, Object *obj,
                            Entity *args) {
  if (!IS_INT(args)) {
    return raise_error(task, ctx, "Expected an int.");
  }
  int64_t micros_since_epoch = pint(&args->pri);
  Timestamp ts = micros_to_timestamp(micros_since_epoch);
  return _timestamp_to_entity(task->parent_process->heap, &ts);
}

void time_add_native(ModuleManager *mm, Module *time) {
  native_function(time, intern("__now_usec"), _now_usec);
  native_function(time, intern("__now_timestamp"), _now_timestamp);
  native_function(time, intern("__timestamp_to_micros"), _timestamp_to_micros);
  native_function(time, intern("__micros_to_timestamp"), _micros_to_timestamp);
}