// array.h
//
// Created on: Aug 22, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_ARRAY_ARRAY_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_ARRAY_ARRAY_H_

#include "c-data-structures/arraylike.h"
#include "zinnia/entity/entity.h"
#include "zinnia/entity/object.h"

DEFINE_ARRAYLIKE(Array, Entity);

void array_init__(Object *obj);
void array_delete__(Object *obj);
void array_print__(const Object *obj, FILE *out);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_ARRAY_ARRAY_H_ */