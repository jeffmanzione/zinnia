// string.h
//
// Created on: Aug 28, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_STRING_STRING_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_STRING_STRING_H_

#include "c-data-structures/arraylike.h"
#include "zinnia/entity/entity.h"
#include "zinnia/entity/object.h"

DEFINE_ARRAYLIKE(String, char);

void String_append_raw(String *const head, const char *tail, uint32_t len);

void string_create__(Object *obj);
void string_init__(Object *obj, const char *str, size_t size);
void string_delete__(Object *obj);
void string_print__(const Object *obj, FILE *out);

typedef struct {
  char *str;
  int len;
} IString;

void istring_create__(Object *obj);
void istring_init__(Object *obj, const char *str, size_t size);
void istring_init_no_intern__(Object *obj, const char *str, size_t size);
void istring_delete__(Object *obj);
void istring_print__(const Object *obj, FILE *out);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_STRING_STRING_H_ */