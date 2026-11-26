// string.c
//
// Created on: Aug 28, 2020
//     Author: Jeff Manzione

#include "zinnia/value/string/string.h"

#include <stdio.h>
#include <string.h>

#include "zinnia/alloc/alloc.h"
#include "zinnia/util/error.h"
#include "zinnia/value/object.h"
#include "zinnia/value/value.h"
#include "zinnia/vm/intern.h"

IMPL_ARRAYLIKE(String, char);

// Fills [string] with [size] chars of [str], or with room for them if [str] is
// NULL, as String_create_copy() and String_create_capacity() would. Those
// leave the table unset when there's no room, so an empty String gets the
// default room instead.
static void string_fill_(String *string, const char *str, size_t size) {
  if (0 == size) {
    String_init(string);
  } else if (NULL == str) {
    String_init_capacity(string, size);
  } else {
    // Rounded up, as String_create_copy() does.
    String_init_capacity(string, ((size + ARRAYLIKE_DEFAULT_TABLE_SIZE - 1) /
                                  ARRAYLIKE_DEFAULT_TABLE_SIZE) *
                                     ARRAYLIKE_DEFAULT_TABLE_SIZE);
    memmove(string->table, str, size);
    string->size = size;
  }
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

void string_init__(Object *obj, String *storage, const char *str, size_t size) {
  string_fill_(storage, str, size);
  obj->_internal_obj = storage;
}

void string_delete__(Object *obj) {
  if (NULL == obj->_internal_obj) {
    return;
  }
  // The heap frees the String itself (see heap_string_storage()).
  String_finalize((String *)obj->_internal_obj);
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