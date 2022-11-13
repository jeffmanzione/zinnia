// primitive.c
//
// Created on: Jun 01, 2020
//     Author: Jeff Manzione

#include "entity/primitive.h"

#include <stdint.h>

#include "debug/debug.h"

PrimitiveType ptype(const Primitive *p) {
  ASSERT(NOT_NULL(p));
  return p->_type;
}

int8_t pchar(const Primitive *p) {
  ASSERT(NOT_NULL(p), p->_type == PRIMITIVE_CHAR);
  return p->_char_val;
}

int64_t pint(const Primitive *p) {
  ASSERT(NOT_NULL(p), p->_type == PRIMITIVE_INT);
  return p->_int_val;
}

double pfloat(const Primitive *p) {
  ASSERT(NOT_NULL(p), p->_type == PRIMITIVE_FLOAT);
  return p->_float_val;
}

void pset_char(Primitive *p, int8_t val) {
  ASSERT(NOT_NULL(p));
  p->_type = PRIMITIVE_CHAR;
  p->_char_val = val;
}

void pset_int(Primitive *p, int64_t val) {
  ASSERT(NOT_NULL(p));
  p->_type = PRIMITIVE_INT;
  p->_int_val = val;
}

void pset_float(Primitive *p, double val) {
  ASSERT(NOT_NULL(p));
  p->_type = PRIMITIVE_FLOAT;
  p->_float_val = val;
}

Primitive primitive_int(int64_t val) {
  Primitive p = {._type = PRIMITIVE_INT, ._int_val = val};
  return p;
}

Primitive primitive_char(int8_t val) {
  Primitive p = {._type = PRIMITIVE_CHAR, ._char_val = val};
  return p;
}

Primitive primitive_float(double val) {
  Primitive p = {._type = PRIMITIVE_FLOAT, ._float_val = val};
  return p;
}

double float_of(const Primitive *p) {
  switch (ptype(p)) {
  case PRIMITIVE_INT:
    return (double)pint(p);
  case PRIMITIVE_CHAR:
    return (double)pchar(p);
  default:
    return pfloat(p);
  }
}

int64_t int_of(const Primitive *p) {
  switch (ptype(p)) {
  case PRIMITIVE_INT:
    return pint(p);
  case PRIMITIVE_CHAR:
    return (int64_t)pchar(p);
  default:
    return (int64_t)pfloat(p);
  }
}

int8_t char_of(const Primitive *p) {
  switch (ptype(p)) {
  case PRIMITIVE_INT:
    return (int8_t)pint(p);
  case PRIMITIVE_CHAR:
    return (int8_t)pchar(p);
  default:
    return (int8_t)pfloat(p);
  }
}

bool primitive_equals(const Primitive *p1, const Primitive *p2) {
  if (PRIMITIVE_FLOAT == ptype(p1) || PRIMITIVE_FLOAT == ptype(p2)) {
    return float_of(p1) == float_of(p2);
  }
  if (PRIMITIVE_INT == ptype(p1) || PRIMITIVE_INT == ptype(p2)) {
    return int_of(p1) == int_of(p2);
  }
  return char_of(p1) == char_of(p2);
}
