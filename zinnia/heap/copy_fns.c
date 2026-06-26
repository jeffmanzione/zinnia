// copy_fns.c
//
// Created on: Dec 26, 2020
//     Author: Jeff Manzione

#include "zinnia/heap/copy_fns.h"

#include "zinnia/entity/array/array.h"
#include "zinnia/entity/entity.h"
#include "zinnia/entity/object.h"
#include "zinnia/entity/string/string.h"
#include "zinnia/entity/tuple/tuple.h"
#include "zinnia/heap/heap.h"

void array_copy(EntityCopier *copier, const Object *src_obj,
                Object *target_obj) {
  Array *src = (Array *)src_obj->_internal_obj;
  size_t size = Array_size(src);
  int i;
  for (i = 0; i < size; ++i) {
    Entity member = entitycopier_copy(copier, Array_get_ref_unchecked(src, i));
    array_add(copier->target, target_obj, &member);
  }
}

void tuple_copy(EntityCopier *copier, const Object *src_obj,
                Object *target_obj) {
  Tuple *src = (Tuple *)src_obj->_internal_obj;
  size_t size = tuple_size(src);
  Tuple *target = tuple_create(size);
  target_obj->_internal_obj = target;
  int i;
  for (i = 0; i < size; ++i) {
    Entity member = entitycopier_copy(copier, tuple_get_mutable(src, i));
    tuple_set(copier->target, target_obj, i, &member);
  }
}

void string_copy(EntityCopier *copier, const Object *src_obj,
                 Object *target_obj) {
  String *src = (String *)src_obj->_internal_obj;
  string_init__(target_obj, src->table, String_size(src));
}

void istring_copy(EntityCopier *copier, const Object *src_obj,
                  Object *target_obj) {
  *(IString *)target_obj->_internal_obj = *(IString *)src_obj->_internal_obj;
}