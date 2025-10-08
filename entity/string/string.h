// string.h
//
// Created on: Aug 28, 2020
//     Author: Jeff Manzione

#ifndef ENTITY_STRING_STRING_H_
#define ENTITY_STRING_STRING_H_

#include "entity/entity.h"
#include "entity/object.h"
#include "struct/arraylike.h"

DEFINE_ARRAYLIKE(String, char);

void String_append_raw(String *const head, const char *tail, uint32_t len);

void __string_create(Object *obj);
void __string_init(Object *obj, const char *str, size_t size);
void __string_delete(Object *obj);
void __string_print(const Object *obj, FILE *out);

typedef struct {
  char *str;
  int len;
} IString;

void __istring_create(Object *obj);
void __istring_init(Object *obj, const char *str, size_t size);
void __istring_init_no_intern(Object *obj, const char *str, size_t size);
void __istring_delete(Object *obj);
void __istring_print(const Object *obj, FILE *out);

#endif /* ENTITY_STRING_STRING_H_ */