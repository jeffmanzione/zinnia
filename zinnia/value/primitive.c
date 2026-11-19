// primitive.c
//
// Created on: Jun 01, 2020
//     Author: Jeff Manzione

#include "zinnia/value/primitive.h"

#include <stdint.h>
#include <stdlib.h>

#include "zinnia/util/error.h"

bool primitive_equals(const Primitive *p1, const Primitive *p2) {
  if (PRIMITIVE_FLOAT == ptype(p1) || PRIMITIVE_FLOAT == ptype(p2)) {
    return float_of(p1) == float_of(p2);
  }
  if (PRIMITIVE_INT == ptype(p1) || PRIMITIVE_INT == ptype(p2)) {
    return int_of(p1) == int_of(p2);
  }
  if (PRIMITIVE_CHAR == ptype(p1) || PRIMITIVE_CHAR == ptype(p2)) {
    return char_of(p1) == char_of(p2);
  }
  return bool_of(p1) == bool_of(p2);
}
