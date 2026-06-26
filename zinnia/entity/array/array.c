// array.c
//
// Created on: Aug 22, 2020
//     Author: Jeff Manzione

#include "zinnia/entity/array/array.h"

#include <stdio.h>

IMPL_ARRAYLIKE(Array, Entity);

void array_init__(Object *obj) { obj->_internal_obj = Array_create(); }

void array_delete__(Object *obj) { Array_delete((Array *)obj->_internal_obj); }

void array_print__(const Object *obj, FILE *out) {
  Array *array = (Array *)obj->_internal_obj;
  fprintf(out, "[");
  if (Array_size(array) > 0) {
    entity_print(Array_get_ref_unchecked(array, 0), out);
  }
  int i;
  for (i = 1; i < Array_size(array); ++i) {
    fprintf(out, ",");
    entity_print(Array_get_ref_unchecked(array, i), out);
  }
  fprintf(out, "]");
}