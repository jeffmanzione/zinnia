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
DEFINE_DATA_ARRAY(Float32Array, float, entity_float);
DEFINE_DATA_ARRAY(Float64Array, double, entity_float);

DEFINE_DATA_MATRIX(Int8Matrix, Int8Array, int8_t, entity_int);
DEFINE_DATA_MATRIX(Int32Matrix, Int32Array, int32_t, entity_int);
DEFINE_DATA_MATRIX(Int64Matrix, Int64Array, int64_t, entity_int);
DEFINE_DATA_MATRIX(Float32Matrix, Float32Array, float, entity_float);
DEFINE_DATA_MATRIX(Float64Matrix, Float64Array, double, entity_float);

void data_add_native(ModuleManager *mm, Module *data) {
  INSTALL_DATA_ARRAY(Int8Array, data);
  INSTALL_DATA_ARRAY(Int32Array, data);
  INSTALL_DATA_ARRAY(Int64Array, data);
  INSTALL_DATA_ARRAY(Float32Array, data);
  INSTALL_DATA_ARRAY(Float64Array, data);

  INSTALL_DATA_MATRIX(Int8Matrix, data);
  INSTALL_DATA_MATRIX(Int32Matrix, data);
  INSTALL_DATA_MATRIX(Int64Matrix, data);
  INSTALL_DATA_MATRIX(Float32Matrix, data);
  INSTALL_DATA_MATRIX(Float64Matrix, data);
}