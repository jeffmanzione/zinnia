// string_helper.h
//
// Created on: Oct 5, 2020
//     Author: Jeff Manzione

#include "entity/object.h"
#include "heap/heap.h"

Object *string_new(Heap *heap, const char src[], size_t len);