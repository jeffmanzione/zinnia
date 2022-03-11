/*
 * tuple.c
 *
 *  Created on: Jan 1, 2017
 *      Author: Dad
 */

#include "entity/tuple/tuple.h"

#include <stddef.h>
#include <stdio.h>

#include "alloc/alloc.h"
#include "debug/debug.h"
#include "entity/entity.h"

struct _Tuple {
  size_t size;
  Entity *table;
};

void tuple_print(const Tuple *t, FILE *file);

void __tuple_create(Object *obj) {}

void __tuple_delete(Object *obj) {
  ASSERT(NOT_NULL(obj));
  Tuple *t = (Tuple *)obj->_internal_obj;
  ASSERT(NOT_NULL(t));
  tuple_delete(t);
}

void __tuple_print(const Object *obj, FILE *out) {
  ASSERT(NOT_NULL(obj));
  Tuple *tuple = (Tuple *)obj->_internal_obj;
  ASSERT(NOT_NULL(tuple));
  tuple_print(tuple, out);
}

Tuple *tuple_create(size_t size) {
  Tuple *t = ALLOC(Tuple);
  t->size = size;
  t->table = ALLOC_ARRAY2(Entity, size);
  return t;
}

Entity *tuple_get_mutable(const Tuple *t, uint32_t index) {
  ASSERT(NOT_NULL(t), index >= 0, index < t->size);
  return t->table + index;
}

const Entity *tuple_get(const Tuple *t, uint32_t index) {
  return tuple_get_mutable(t, index);
}

uint32_t tuple_size(const Tuple *t) {
  ASSERT_NOT_NULL(t);
  return t->size;
}

void tuple_delete(Tuple *t) {
  if (t->size > 0) {
    DEALLOC(t->table);
  }
  DEALLOC(t);
}

void tuple_print(const Tuple *t, FILE *file) {
  ASSERT(NOT_NULL(t), NOT_NULL(file));
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