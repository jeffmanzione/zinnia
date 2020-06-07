// primitive.h
//
// Created on: Jun 01, 2020
//     Author: Jeff Manzione

#ifndef ENTITY_PRIMITIVE_H_
#define ENTITY_PRIMITIVE_H_

#include <stdint.h>

// Contains a primitive type, i.e.,
//    char: int8_t
//     int: int32_t
//   float: double
typedef enum { CHAR, INT, FLOAT } PrimitiveType;

typedef struct {
  PrimitiveType _type;
  union {
    int8_t _char_val;
    int32_t _int_val;
    double _float_val;
  };
} Primitive;

// Gets the type from the primitive.
PrimitiveType ptype(const Primitive *p);
// Gets the char value of the primitve.
int8_t pchar(const Primitive *p);
// Gets the int value of the primitve.
int32_t pint(const Primitive *p);
// Gets the float value of the primitve.
double pfloat(const Primitive *p);

#endif /* ENTITY_PRIMITIVE_H_ */