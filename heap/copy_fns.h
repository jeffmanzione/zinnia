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

void array_copy(Heap *heap, Map *cpy_map, Object *target_obj, Object *src_obj);
void tuple_copy(Heap *heap, Map *cpy_map, Object *target_obj, Object *src_obj);
void string_copy(Heap *heap, Map *cpy_map, Object *target_obj, Object *src_obj);
void named_list_copy(Heap *heap, Map *cpy_man, Object *target_obj,
                     Object *src_obj);
#endif /* HEAP_COPY_FNS_H_ */