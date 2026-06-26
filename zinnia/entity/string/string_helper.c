// string_helper.c
//
// Created on: Oct 5, 2020
//     Author: Jeff Manzione

#include "zinnia/entity/string/string_helper.h"

#include "language-tools/intern.h"
#include "zinnia/alloc/alloc.h"
#include "zinnia/entity/class/classes_def.h"
#include "zinnia/entity/string/string.h"


Object *string_new(Heap *heap, const char src[], size_t len) {
  Object *str = heap_new(heap, Class_String);
  string_init__(str, src, len);
  return str;
}

Object *istring_new(Heap *heap, const char src[], size_t len) {
  Object *str = heap_new(heap, Class_IString);
  istring_init__(str, src, len);
  return str;
}

Object *istring_new_no_intern(Heap *heap, const char src[], size_t len) {
  Object *str = heap_new(heap, Class_IString);
  istring_init_no_intern__(str, src, len);
  return str;
}

bool extract_string(const Entity *e, char **raw_str, int *len) {
  if (IS_CLASS(e, Class_String)) {
    const String *str = (String *)e->obj->_internal_obj;
    *raw_str = str->table;
    *len = String_size(str);
    return true;
  }
  if (IS_CLASS(e, Class_IString)) {
    const IString *istr = (IString *)e->obj->_internal_obj;
    *raw_str = istr->str;
    *len = istr->len;
    return true;
  }
  *raw_str = NULL;
  *len = -1;
  return false;
}

bool extract_string_obj(const Object *obj, char **raw_str, int *len) {
  if (obj->_class == Class_String) {
    const String *str = (String *)obj->_internal_obj;
    *raw_str = str->table;
    *len = String_size(str);
    return true;
  }
  if (obj->_class == Class_IString) {
    const IString *istr = (IString *)obj->_internal_obj;
    *raw_str = istr->str;
    *len = istr->len;
    return true;
  }
  *raw_str = NULL;
  *len = -1;
  return false;
}

const char *intern_entity(const Entity *e) {
  if (IS_CLASS(e, Class_String)) {
    const String *str = (String *)e->obj->_internal_obj;
    return global_intern_range(str->table, 0, String_size(str));
  }
  if (IS_CLASS(e, Class_IString)) {
    const IString *istr = (IString *)e->obj->_internal_obj;
    return istr->str;
  }
  return NULL;
}

int entity_string_len(const Entity *e) {
  if (IS_CLASS(e, Class_String)) {
    const String *str = (String *)e->obj->_internal_obj;
    return String_size(str);
  }
  if (IS_CLASS(e, Class_IString)) {
    const IString *istr = (IString *)e->obj->_internal_obj;
    return istr->len;
  }
  return -1;
}

char *entity_string_copy(const Entity *e) {
  char *str;
  int len;
  extract_string(e, &str, &len);
  return ALLOC_STRNDUP(str, len);
}