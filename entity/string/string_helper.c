// string_helper.c
//
// Created on: Oct 5, 2020
//     Author: Jeff Manzione

#include "entity/string/string_helper.h"

#include "entity/class/classes_def.h"
#include "entity/string/string.h"

Object *string_new(Heap *heap, const char src[], size_t len) {
  Object *str = heap_new(heap, Class_String);
  __string_init(str, src, len);
  return str;
}