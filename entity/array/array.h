// array.h
//
// Created on: Aug 22, 2020
//     Author: Jeff Manzione

#ifndef ENTITY_ARRAY_ARRAY_H_
#define ENTITY_ARRAY_ARRAY_H_

#include "entity/entity.h"
#include "entity/object.h"
#include "struct/arraylike.h"

DEFINE_ARRAYLIKE(Array, Entity);

void __array_init(Object *obj);
void __array_delete(Object *obj);
void __array_print(const Object *obj, FILE *out);

#endif /* ENTITY_ARRAY_ARRAY_H_ */