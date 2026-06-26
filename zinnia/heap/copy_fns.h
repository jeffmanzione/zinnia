// copy_fns.h
//
// Created on: Dec 26, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_HEAP_COPY_FNS_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_HEAP_COPY_FNS_H_

#include "zinnia/entity/entity.h"
#include "zinnia/entity/object.h"
#include "zinnia/heap/heap.h"

void array_copy(EntityCopier *copier, const Object *src_obj,
                Object *target_obj);
void tuple_copy(EntityCopier *copier, const Object *src_obj,
                Object *target_obj);
void string_copy(EntityCopier *copier, const Object *src_obj,
                 Object *target_obj);
void istring_copy(EntityCopier *copier, const Object *src_obj,
                  Object *target_obj);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_HEAP_COPY_FNS_H_ */