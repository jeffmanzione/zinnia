// string.c
//
// Created on: Aug 28, 2020
//     Author: Jeff Manzione

#include "zinnia/entity/string/string.h"

#include <stdio.h>

#include "language-tools/intern.h"
#include "stdio.h"
#include "zinnia/alloc/alloc.h"
#include "zinnia/entity/entity.h"
#include "zinnia/entity/object.h"
#include "zinnia/util/error.h"

IMPL_ARRAYLIKE(String, char);

void String_append_raw(String *const head, const char *tail, uint32_t len) {
  ASSERT(head != NULL);
  ASSERT(tail != NULL);
  ASSERT(len >= 0);
  // TODO: Implement more efficient version that doesn't make a copy in
  // arraylike.h
  String *tmp = String_create_copy(tail, len);
  String_append(head, tmp);
  String_delete(tmp);
}

void string_create__(Object *obj) { obj->_internal_obj = NULL; }

void string_init__(Object *obj, const char *str, size_t size) {
  String *string = (NULL == str) ? String_create_capacity(size)
                                 : String_create_copy(str, size);
  obj->_internal_obj = string;
}

void string_delete__(Object *obj) {
  if (NULL == obj->_internal_obj) {
    return;
  }
  String_delete((String *)obj->_internal_obj);
}

void string_print__(const Object *obj, FILE *out) {
  String *str = (String *)obj->_internal_obj;
  fprintf(out, "'%*s'", (int)String_size(str), str->table);
}

void istring_create__(Object *obj) { obj->_internal_obj = NULL; }

void istring_init__(Object *obj, const char *str, size_t size) {
  istring_init_no_intern__(obj, global_intern(str), size);
}

void istring_init_no_intern__(Object *obj, const char *str, size_t size) {
  IString *istr = MNEW(IString);
  istr->str = (char *)str;
  istr->len = size;
  obj->_internal_obj = istr;
}

void istring_delete__(Object *obj) {
  if (NULL == obj->_internal_obj) {
    return;
  }
  RELEASE((IString *)obj->_internal_obj);
}

void istring_print__(const Object *obj, FILE *out) {
  IString *istr = (IString *)obj->_internal_obj;
  fprintf(out, "i'%*s'", istr->len, istr->str);
}