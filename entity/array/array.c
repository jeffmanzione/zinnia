// array.c
//
// Created on: Aug 22, 2020
//     Author: Jeff Manzione

#include "entity/array/array.h"

#include <stdio.h>

#include "entity/entity.h"
#include "entity/object.h"
#include "stdio.h"
#include "struct/arraylike.h"

IMPL_ARRAYLIKE(Array, Entity);

void __array_init(Object *obj) { obj->_internal_obj = Array_create(); }

void __array_delete(Object *obj) { Array_delete((Array *)obj->_internal_obj); }

void __array_print(const Object *obj, FILE *out) {
  Array *array = (Array *)obj->_internal_obj;
  fprintf(out, "[");
  if (Array_size(array) > 0) {
    entity_print(Array_get_ref(array, 0), out);
  }
  int i;
  for (i = 1; i < Array_size(array); ++i) {
    fprintf(out, ",");
    entity_print(Array_get_ref(array, i), out);
  }
  fprintf(out, "]");
}