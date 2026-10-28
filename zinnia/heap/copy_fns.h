// copy_fns.h
//
// Created on: Dec 26, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_HEAP_COPY_FNS_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_HEAP_COPY_FNS_H_

#include "zinnia/entity/object.h"
#include "zinnia/entity/value.h"
#include "zinnia/heap/heap.h"

void array_copy(ValueCopier *copier, const Object *src_obj, Object *target_obj);
void tuple_copy(ValueCopier *copier, const Object *src_obj, Object *target_obj);
void string_copy(ValueCopier *copier, const Object *src_obj,
                 Object *target_obj);
void istring_copy(ValueCopier *copier, const Object *src_obj,
                  Object *target_obj);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_HEAP_COPY_FNS_H_ */