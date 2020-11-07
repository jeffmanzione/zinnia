// primitive.h
//
// Created on: Jun 01, 2020
//     Author: Jeff Manzione

#ifndef ENTITY_PRIMITIVE_H_
#define ENTITY_PRIMITIVE_H_

#include <stdbool.h>
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

void pset_char(Primitive *p, int8_t val);
void pset_int(Primitive *p, int32_t val);
void pset_float(Primitive *p, double val);

Primitive primitive_int(int32_t val);
Primitive primitive_char(int8_t val);
Primitive primitive_float(double val);

double float_of(const Primitive *p);
int32_t int_of(const Primitive *p);
int8_t char_of(const Primitive *p);
bool primitive_equals(const Primitive *p1, const Primitive *p2);

#endif /* ENTITY_PRIMITIVE_H_ */