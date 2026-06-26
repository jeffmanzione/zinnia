// time.c
//
// Created on: Nov 12, 2022
//     Author: Jeff Manzione

#include "zinnia/entity/native/time.h"

#include "language-tools/intern.h"
#include "zinnia/entity/class/classes_def.h"
#include "zinnia/entity/entity.h"
#include "zinnia/entity/native/error.h"
#include "zinnia/entity/native/native.h"
#include "zinnia/entity/native/native_helpers.h"
#include "zinnia/entity/object.h"
#include "zinnia/entity/tuple/tuple.h"
#include "zinnia/heap/heap.h"
#include "zinnia/util/time.h"


#define USEC_PER_SEC 1000 * 1000;

Entity now_usec_(Task *task, Context *ctx, Object *obj, Entity *args) {
  return entity_int(current_usec_since_epoch());
}

Entity timestamp_to_entity_(Heap *heap, Timestamp *ts) {
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

Entity now_timestamp_(Task *task, Context *ctx, Object *obj, Entity *args) {
  Timestamp ts = current_local_timestamp();
  return timestamp_to_entity_(task->parent_process->heap, &ts);
}

Entity timestamp_to_micros_(Task *task, Context *ctx, Object *obj,
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

Entity micros_to_timestamp_(Task *task, Context *ctx, Object *obj,
                            Entity *args) {
  if (!IS_INT(args)) {
    return raise_error(task, ctx, "Expected an int.");
  }
  int64_t micros_since_epoch = pint(&args->pri);
  Timestamp ts = micros_to_timestamp(micros_since_epoch);
  return timestamp_to_entity_(task->parent_process->heap, &ts);
}

void time_add_native(ModuleManager *mm, Module *time) {
  native_function(time, global_intern("__now_usec"), now_usec_);
  native_function(time, global_intern("__now_timestamp"), now_timestamp_);
  native_function(time, global_intern("__timestamp_to_micros"),
                  timestamp_to_micros_);
  native_function(time, global_intern("__micros_to_timestamp"),
                  micros_to_timestamp_);
}