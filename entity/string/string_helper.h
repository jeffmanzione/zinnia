// string_helper.h
//
// Created on: Oct 5, 2020
//     Author: Jeff Manzione

#include "entity/object.h"
#include "heap/heap.h"

Object *string_new(Heap *heap, const char src[], size_t len);
Object *istring_new(Heap *heap, const char src[], size_t len);
Object *istring_new_no_intern(Heap *heap, const char src[], size_t len);
bool extract_string(const Entity *e, char **raw_str, int *len);
bool extract_string_obj(const Object *obj, char **raw_str, int *len);
char *entity_string_copy(const Entity *e);
const char *intern_entity(const Entity *e);
int entity_string_len(const Entity *e);