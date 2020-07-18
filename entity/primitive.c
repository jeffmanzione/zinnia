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

void pset_char(Primitive *p, int8_t val) {
  ASSERT(NOT_NULL(p));
  p->_type = CHAR;
  p->_char_val = val;
}

void pset_int(Primitive *p, int32_t val) {
  ASSERT(NOT_NULL(p));
  p->_type = INT;
  p->_int_val = val;
}

void pset_float(Primitive *p, double val) {
  ASSERT(NOT_NULL(p));
  p->_type = FLOAT;
  p->_float_val = val;
}