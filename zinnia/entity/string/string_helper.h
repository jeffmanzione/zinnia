// string_helper.h
//
// Created on: Oct 5, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_STRING_STRING_HELPER_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_STRING_STRING_HELPER_H_

#include "zinnia/entity/object.h"
#include "zinnia/heap/heap.h"

Object *string_new(Heap *heap, const char src[], size_t len);
Object *istring_new(Heap *heap, const char src[], size_t len);
Object *istring_new_no_intern(Heap *heap, const char src[], size_t len);
bool extract_string(const Entity *e, char **raw_str, int *len);
bool extract_string_obj(const Object *obj, char **raw_str, int *len);
char *entity_string_copy(const Entity *e);
const char *intern_entity(const Entity *e);
int entity_string_len(const Entity *e);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_STRING_STRING_HELPER_H_ */