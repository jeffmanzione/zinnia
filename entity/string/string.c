// string.c
//
// Created on: Aug 28, 2020
//     Author: Jeff Manzione

#include "entity/string/string.h"

#include <stdio.h>

#include "debug/debug.h"
#include "entity/entity.h"
#include "entity/object.h"
#include "stdio.h"
#include "struct/arraylike.h"

IMPL_ARRAYLIKE(String, char);

void __string_create(Object *obj) { obj->_internal_obj = NULL; }

void __string_init(Object *obj, const char *str, size_t size) {
  // What is the runtime of strlen?
  String *string = String_create_copy(str, size);
  obj->_internal_obj = string;
}

void __string_delete(Object *obj) {
  if (NULL == obj->_internal_obj) {
    return;
  }
  String_delete((String *)obj->_internal_obj);
}

void __string_print(const Object *obj, FILE *out) {
  String *str = (String *)obj->_internal_obj;
  fprintf(out, "'%*s'", String_size(str), str->table);
}