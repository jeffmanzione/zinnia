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

void __string_create(Object *obj);
void __string_init(Object *obj, const char *str, size_t size);
void __string_delete(Object *obj);
void __string_print(const Object *obj, FILE *out);

#endif /* ENTITY_STRING_STRING_H_ */