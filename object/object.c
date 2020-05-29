// object.c
//
// Created on: May 28, 2020
//     Author: Jeff Manzione

#include "object/object.h"

#include "struct/map.h"

struct _Primitive {
  enum { CHARACTER, INTEGER, FLOATING } type;
  union {
    int8_t char_val;
    int32_t int_val;
    double float_val;
  };
};

struct _Object {
  Class *class;
  Map members;
};

struct _Record {
  enum { NONE, PRIMITIVE, OBJECT } type;
  union {
    Primitive pri;
    Object *obj;
  };
};

struct _Class {
  const char *name;
  Module *module;
  Object *prototype;
};
