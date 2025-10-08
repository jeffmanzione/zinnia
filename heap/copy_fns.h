// copy_fns.h
//
// Created on: Dec 26, 2020
//     Author: Jeff Manzione

#ifndef HEAP_COPY_FNS_H_
#define HEAP_COPY_FNS_H_

#include "entity/entity.h"
#include "entity/object.h"
#include "heap/heap.h"
#include "struct/map.h"

void array_copy(EntityCopier *copier, const Object *src_obj,
                Object *target_obj);
void tuple_copy(EntityCopier *copier, const Object *src_obj,
                Object *target_obj);
void string_copy(EntityCopier *copier, const Object *src_obj,
                 Object *target_obj);
void istring_copy(EntityCopier *copier, const Object *src_obj,
                  Object *target_obj);

#endif /* HEAP_COPY_FNS_H_ */