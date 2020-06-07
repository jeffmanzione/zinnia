// primitive.c
//
// Created on: Jun 01, 2020
//     Author: Jeff Manzione

#include "entity/primitive.h"

#include <stdint.h>

#include "debug/debug.h"

inline PrimitiveType ptype(const Primitive *p) {
  ASSERT(NOT_NULL(p));
  return p->_type;
}

int8_t pchar(const Primitive *p) {
  ASSERT(NOT_NULL(p), p->_type == CHAR);
  return p->_char_val;
}

int32_t pint(const Primitive *p) {
  ASSERT(NOT_NULL(p), p->_type == INT);
  return p->_int_val;
}

double pfloat(const Primitive *p) {
  ASSERT(NOT_NULL(p), p->_type == FLOAT);
  return p->_float_val;
}