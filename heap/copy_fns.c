// copy_fns.c
//
// Created on: Dec 26, 2020
//     Author: Jeff Manzione

#include "heap/copy_fns.h"

#include "entity/array/array.h"
#include "entity/entity.h"
#include "entity/object.h"
#include "entity/string/string.h"
#include "entity/tuple/tuple.h"
#include "heap/heap.h"

void array_copy(EntityCopier *copier, const Object *src_obj,
                Object *target_obj) {
  Array *src = (Array *)src_obj->_internal_obj;
  size_t size = Array_size(src);
  int i;
  for (i = 0; i < size; ++i) {
    Entity member = entitycopier_copy(copier, Array_get_ref(src, i));
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
  __string_init(target_obj, src->table, String_size(src));
}

void istring_copy(EntityCopier *copier, const Object *src_obj,
                  Object *target_obj) {
  *(IString *)target_obj->_internal_obj = *(IString *)src_obj->_internal_obj;
}