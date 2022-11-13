// time.c
//
// Created on: Nov 12, 2022
//     Author: Jeff Manzione

#include "entity/native/time.h"

#include "alloc/arena/intern.h"
#include "entity/entity.h"
#include "entity/native/error.h"
#include "entity/native/native.h"
#include "entity/object.h"
#include "heap/heap.h"
#include "util/time.h"

#define USEC_PER_SEC 1000 * 1000;

Entity _now_usec(Task *task, Context *ctx, Object *obj, Entity *args) {
  return entity_int(current_usec_since_epoch());
}

Entity _millis_to_timestamp(Task *task, Context *ctx, Object *obj,
                            Entity *args) {
  if (!IS_INT(args)) {
    return raise_error(task, ctx, "Expected an Int arg.");
  }
  uint32_t millis_since_epoch = pint(&args->pri);
  Timestamp ts = timestamp_from_millis(millis_since_epoch);
  Entity year = entity_int(ts.year);
  Entity month = entity_int(ts.month);
  Entity day_of_month = entity_int(ts.day_of_month);
  Entity hour = entity_int(ts.hour);
  Entity minute = entity_int(ts.minute);
  Entity second = entity_int(ts.second);
  Entity millisecond = entity_int(ts.millisecond);
  return entity_object(tuple_create7(task->parent_process->heap, &year, &month,
                                     &day_of_month, &hour, &minute, &second,
                                     &millisecond));
}

void time_add_native(ModuleManager *mm, Module *time) {
  native_function(time, intern("now_usec"), _now_usec);
  native_function(time, intern("__millis_to_timestamp"), _millis_to_timestamp);
}