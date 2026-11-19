// primitive.h
//
// Created on: Jun 01, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_VALUE_PRIMITIVE_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_VALUE_PRIMITIVE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "zinnia/util/error.h"

typedef enum {
  PRIMITIVE_BOOL,
  PRIMITIVE_CHAR,
  PRIMITIVE_INT,
  PRIMITIVE_FLOAT
} PrimitiveType;

typedef struct {
  PrimitiveType _type;
  union {
    bool _bool_val;
    int8_t _char_val;
    int64_t _int_val;
    double _float_val;
  };
} Primitive;

// Gets the type from the primitive.
// These are inline, since the VM uses them for nearly every instruction.
static inline PrimitiveType ptype(const Primitive *p) {
  ASSERT(p != NULL);
  return p->_type;
}

static inline bool pbool(const Primitive *p) {
  ASSERT(p != NULL);
  ASSERT(p->_type == PRIMITIVE_BOOL);
  return p->_bool_val;
}

static inline int8_t pchar(const Primitive *p) {
  ASSERT(p != NULL);
  ASSERT(p->_type == PRIMITIVE_CHAR);
  return p->_char_val;
}

static inline int64_t pint(const Primitive *p) {
  ASSERT(p != NULL);
  ASSERT(p->_type == PRIMITIVE_INT);
  return p->_int_val;
}

static inline double pfloat(const Primitive *p) {
  ASSERT(p != NULL);
  ASSERT(p->_type == PRIMITIVE_FLOAT);
  return p->_float_val;
}

static inline void pset_bool(Primitive *p, bool val) {
  ASSERT(p != NULL);
  p->_type = PRIMITIVE_BOOL;
  p->_bool_val = val;
}

static inline void pset_char(Primitive *p, int8_t val) {
  ASSERT(p != NULL);
  p->_type = PRIMITIVE_CHAR;
  p->_char_val = val;
}

static inline void pset_int(Primitive *p, int64_t val) {
  ASSERT(p != NULL);
  p->_type = PRIMITIVE_INT;
  p->_int_val = val;
}

static inline void pset_float(Primitive *p, double val) {
  ASSERT(p != NULL);
  p->_type = PRIMITIVE_FLOAT;
  p->_float_val = val;
}

static inline Primitive primitive_bool(bool val) {
  Primitive p = {._type = PRIMITIVE_BOOL, ._bool_val = val};
  return p;
}

static inline Primitive primitive_char(int8_t val) {
  Primitive p = {._type = PRIMITIVE_CHAR, ._char_val = val};
  return p;
}

static inline Primitive primitive_int(int64_t val) {
  Primitive p = {._type = PRIMITIVE_INT, ._int_val = val};
  return p;
}

static inline Primitive primitive_float(double val) {
  Primitive p = {._type = PRIMITIVE_FLOAT, ._float_val = val};
  return p;
}

static inline bool bool_of(const Primitive *p) {
  switch (ptype(p)) {
    case PRIMITIVE_BOOL:
      return pbool(p);
    case PRIMITIVE_CHAR:
      return (bool)pchar(p);
    case PRIMITIVE_INT:
      return (bool)pint(p);
    default:
      return (bool)pfloat(p);
  }
}

static inline int8_t char_of(const Primitive *p) {
  switch (ptype(p)) {
    case PRIMITIVE_BOOL:
      return (int8_t)pbool(p);
    case PRIMITIVE_CHAR:
      return pchar(p);
    case PRIMITIVE_INT:
      return (int8_t)pint(p);
    default:
      return (int8_t)pfloat(p);
  }
}

static inline int64_t int_of(const Primitive *p) {
  switch (ptype(p)) {
    case PRIMITIVE_BOOL:
      return (int64_t)pbool(p);
    case PRIMITIVE_CHAR:
      return (int64_t)pchar(p);
    case PRIMITIVE_INT:
      return pint(p);
    default:
      return (int64_t)pfloat(p);
  }
}

static inline double float_of(const Primitive *p) {
  switch (ptype(p)) {
    case PRIMITIVE_BOOL:
      return (double)pbool(p);
    case PRIMITIVE_CHAR:
      return (double)pchar(p);
    case PRIMITIVE_INT:
      return (double)pint(p);
    default:
      return pfloat(p);
  }
}

bool primitive_equals(const Primitive *p1, const Primitive *p2);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_VALUE_PRIMITIVE_H_ */