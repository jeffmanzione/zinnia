/*
 * tuple.c
 *
 *  Created on: Jan 1, 2017
 *      Author: Dad
 */

#include "zinnia/entity/tuple/tuple.h"

#include <stddef.h>
#include <stdio.h>

#include "zinnia/alloc/alloc.h"
#include "zinnia/entity/entity.h"
#include "zinnia/util/error.h"

struct _Tuple {
  size_t size;
  Entity *table;
};

void tuple_print(const Tuple *t, FILE *file);

void __tuple_create(Object *obj) {}

void __tuple_delete(Object *obj) {
  ASSERT(obj != NULL);
  Tuple *t = (Tuple *)obj->_internal_obj;
  ASSERT(t != NULL);
  tuple_delete(t);
}

void __tuple_print(const Object *obj, FILE *out) {
  ASSERT(obj != NULL);
  Tuple *tuple = (Tuple *)obj->_internal_obj;
  ASSERT(tuple != NULL);
  tuple_print(tuple, out);
}

Tuple *tuple_create(size_t size) {
  Tuple *t = CNEW(Tuple);
  t->size = size;
  t->table = MNEW_ARR(Entity, size);
  return t;
}

Entity *tuple_get_mutable(const Tuple *t, uint32_t index) {
  ASSERT(t != NULL);
  ASSERT(index >= 0);
  ASSERT(index < t->size);
  return t->table + index;
}

const Entity *tuple_get(const Tuple *t, uint32_t index) {
  return tuple_get_mutable(t, index);
}

uint32_t tuple_size(const Tuple *t) {
  ASSERT(t != NULL);
  return t->size;
}

void tuple_delete(Tuple *t) {
  if (t->size > 0) {
    RELEASE(t->table);
  }
  RELEASE(t);
}

void tuple_print(const Tuple *t, FILE *file) {
  ASSERT(t != NULL);
  ASSERT(file != NULL);
  fprintf(file, "(");
  if (t->size > 0) {
    entity_print(tuple_get(t, 0), file);
    int i;
    for (i = 1; i < t->size; ++i) {
      fprintf(file, ", ");
      entity_print(tuple_get(t, i), file);
    }
  }
  fprintf(file, ")");
}