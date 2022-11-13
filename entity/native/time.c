// time.c
//
// Created on: Nov 12, 2022
//     Author: Jeff Manzione

#include "entity/native/time.h"

#include "alloc/arena/intern.h"
#include "entity/entity.h"
#include "entity/native/native.h"
#include "entity/object.h"
#include "util/time.h"

#define USEC_PER_SEC 1000 * 1000;

Entity _now_usec(Task *task, Context *ctx, Object *obj, Entity *args) {
  return entity_int(current_usec_since_epoch());
}

void time_add_native(ModuleManager *mm, Module *time) {
  native_function(time, intern("now_usec"), _now_usec);
}