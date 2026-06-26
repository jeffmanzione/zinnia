// tuple.h
//
// Created on: Aug 29, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_TUPLE_TUPLE_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_TUPLE_TUPLE_H_

#include <stddef.h>
#include <stdio.h>

#include "zinnia/entity/entity.h"

typedef struct _Tuple Tuple;

void __tuple_create(Object *obj);
void __tuple_delete(Object *obj);
void __tuple_print(const Object *obj, FILE *out);

Tuple *tuple_create(size_t size);
Entity *tuple_get_mutable(const Tuple *t, uint32_t index);
const Entity *tuple_get(const Tuple *t, uint32_t index);
uint32_t tuple_size(const Tuple *t);
void tuple_delete(Tuple *t);
void tuple_print(const Tuple *t, FILE *file);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_TUPLE_TUPLE_H_ */