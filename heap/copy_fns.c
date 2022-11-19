// copy_fns.c
//
// Created on: Dec 26, 2020
//     Author: Jeff Manzione

#include "heap/copy_fns.h"

#include "entity/array/array.h"
#include "entity/entity.h"
#include "entity/named_list/named_list.h"
#include "entity/object.h"
#include "entity/string/string.h"
#include "entity/tuple/tuple.h"
#include "heap/heap.h"

void array_copy(Heap *heap, Map *cpy_map, Object *target_obj, Object *src_obj) {
  Array *src = (Array *)src_obj->_internal_obj;
  size_t size = Array_size(src);
  int i;
  for (i = 0; i < size; ++i) {
    Entity member = entity_copy(heap, cpy_map, Array_get_ref(src, i));
    array_add(heap, target_obj, &member);
  }
}

void tuple_copy(Heap *heap, Map *cpy_map, Object *target_obj, Object *src_obj) {
  Tuple *src = (Tuple *)src_obj->_internal_obj;
  size_t size = tuple_size(src);
  Tuple *target = tuple_create(size);
  target_obj->_internal_obj = target;
  int i;
  for (i = 0; i < size; ++i) {
    Entity member = entity_copy(heap, cpy_map, tuple_get_mutable(src, i));
    tuple_set(heap, target_obj, i, &member);
  }
}

void string_copy(Heap *heap, Map *cpy_map, Object *target_obj,
                 Object *src_obj) {
  String *src = (String *)src_obj->_internal_obj;
  __string_init(target_obj, src->table, String_size(src));
}

void named_list_copy(Heap *heap, Map *cpy_map, Object *target_obj,
                     Object *src_obj) {
  NamedList *src = (NamedList *)src_obj->_internal_obj;
  KL_iter iter = named_list_iter(src);
  for (; kl_has(&iter); kl_inc(&iter)) {
    Entity copy = entity_copy(heap, cpy_map, kl_value(&iter));
    named_list_set(heap, target_obj, kl_key(&iter), &copy);
  }
}
