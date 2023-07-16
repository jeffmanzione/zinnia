// data.c
//
// Created on: July 7, 2023
//     Author: Jeff Manzione

#include "entity/native/data.h"

#include <stdint.h>

#include "entity/native/data_helper.h"

DEFINE_DATA_ARRAY(Int8Array, int8_t, entity_int);
DEFINE_DATA_ARRAY(Int32Array, int32_t, entity_int);
DEFINE_DATA_ARRAY(Int64Array, int64_t, entity_int);
DEFINE_DATA_ARRAY(Float64Array, double, entity_float);

void data_add_native(ModuleManager *mm, Module *data) {
  INSTALL_DATA_ARRAY(Int8Array);
  INSTALL_DATA_ARRAY(Int32Array);
  INSTALL_DATA_ARRAY(Int64Array);
  INSTALL_DATA_ARRAY(Float64Array);
}