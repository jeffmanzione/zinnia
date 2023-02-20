// primitive.h
//
// Created on: Jun 01, 2020
//     Author: Jeff Manzione

#ifndef ENTITY_PRIMITIVE_H_
#define ENTITY_PRIMITIVE_H_

#include <stdbool.h>
#include <stdint.h>

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
PrimitiveType ptype(const Primitive *p);
// Gets the bool value of the primitve.
bool pbool(const Primitive *p);
// Gets the char value of the primitve.
int8_t pchar(const Primitive *p);
// Gets the int value of the primitve.
int64_t pint(const Primitive *p);
// Gets the float value of the primitve.
double pfloat(const Primitive *p);

void pset_bool(Primitive *p, bool val);
void pset_char(Primitive *p, int8_t val);
void pset_int(Primitive *p, int64_t val);
void pset_float(Primitive *p, double val);

Primitive primitive_bool(bool val);
Primitive primitive_char(int8_t val);
Primitive primitive_int(int64_t val);
Primitive primitive_float(double val);

bool bool_of(const Primitive *p);
int8_t char_of(const Primitive *p);
int64_t int_of(const Primitive *p);
double float_of(const Primitive *p);
bool primitive_equals(const Primitive *p1, const Primitive *p2);

#endif /* ENTITY_PRIMITIVE_H_ */