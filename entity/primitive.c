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

inline int8_t pchar(const Primitive *p) {
  ASSERT(NOT_NULL(p), p->_type == CHAR);
  return p->_char_val;
}

inline int32_t pint(const Primitive *p) {
  ASSERT(NOT_NULL(p), p->_type == INT);
  return p->_int_val;
}

inline double pfloat(const Primitive *p) {
  ASSERT(NOT_NULL(p), p->_type == FLOAT);
  return p->_float_val;
}

inline void pset_char(Primitive *p, int8_t val) {
  ASSERT(NOT_NULL(p));
  p->_type = CHAR;
  p->_char_val = val;
}

inline void pset_int(Primitive *p, int32_t val) {
  ASSERT(NOT_NULL(p));
  p->_type = INT;
  p->_int_val = val;
}

inline void pset_float(Primitive *p, double val) {
  ASSERT(NOT_NULL(p));
  p->_type = FLOAT;
  p->_float_val = val;
}

Primitive primitive_int(int32_t val) {
  Primitive p = {._type = INT, ._int_val = val};
  return p;
}

Primitive primitive_char(int8_t val) {
  Primitive p = {._type = CHAR, ._char_val = val};
  return p;
}

Primitive primitive_float(double val) {
  Primitive p = {._type = FLOAT, ._float_val = val};
  return p;
}

inline double float_of(const Primitive *p) {
  switch (ptype(p)) {
  case INT:
    return (double)pint(p);
  case CHAR:
    return (double)pchar(p);
  default:
    return pfloat(p);
  }
}

inline int32_t int_of(const Primitive *p) {
  switch (ptype(p)) {
  case INT:
    return pint(p);
  case CHAR:
    return (int32_t)pchar(p);
  default:
    return (int32_t)pfloat(p);
  }
}

inline int8_t char_of(const Primitive *p) {
  switch (ptype(p)) {
  case INT:
    return (int8_t)pint(p);
  case CHAR:
    return (int8_t)pchar(p);
  default:
    return (int8_t)pfloat(p);
  }
}

inline bool primitive_equals(const Primitive *p1, const Primitive *p2) {
  if (FLOAT == ptype(p1) || FLOAT == ptype(p2)) {
    return float_of(p1) == float_of(p2);
  }
  if (INT == ptype(p1) || INT == ptype(p2)) {
    return int_of(p1) == int_of(p2);
  }
  return char_of(p1) == char_of(p2);
}
