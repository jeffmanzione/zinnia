// string.c
//
// Created on: Aug 28, 2020
//     Author: Jeff Manzione

#include "zinnia/value/string/string.h"

#include <stdio.h>

#include "zinnia/alloc/alloc.h"
#include "zinnia/util/error.h"
#include "zinnia/value/object.h"
#include "zinnia/value/value.h"
#include "zinnia/vm/intern.h"

IMPL_ARRAYLIKE(String, char);

// String_create_capacity() and String_create_copy() leave a String's table
// unset when it has no room, so an empty String gets the default room instead.
static String *string_create_(const char *str, size_t size) {
  if (0 == size) {
    return String_create();
  }
  return (NULL == str) ? String_create_capacity(size)
                       : String_create_copy(str, size);
}

void String_append_raw(String *const head, const char *tail, uint32_t len) {
  ASSERT(head != NULL);
  ASSERT(tail != NULL);
  ASSERT(len >= 0);
  if (0 == len) {
    return;
  }
  // TODO: Implement more efficient version that doesn't make a copy in
  // arraylike.h
  String *tmp = String_create_copy(tail, len);
  String_append(head, tmp);
  String_delete(tmp);
}

void string_create__(Object *obj) { obj->_internal_obj = NULL; }

void string_init__(Object *obj, const char *str, size_t size) {
  obj->_internal_obj = string_create_(str, size);
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