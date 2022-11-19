// named_list.h
//
// Created on: Nov 18, 2022
//     Author: Jeff Manzione

#ifndef ENTITY_NAMED_LIST_NAMED_LIST_H_
#define ENTITY_NAMED_LIST_NAMED_LIST_H_

#include <stdio.h>

#include "entity/entity.h"
#include "struct/keyed_list.h"

typedef struct _NamedList NamedList;

void __named_list_create(Object *obj);
void __named_list_delete(Object *obj);
void __named_list_print(const Object *obj, FILE *out);

NamedList *named_list_create();

Entity *named_list_get_mutable(NamedList *nl, const char key[]);

const Entity *named_list_get(const NamedList *nl, const char key[]);

Entity *named_list_insert(NamedList *nl, const char key[], Entity **entry_pos);

void named_list_delete(NamedList *nl);

void named_list_print(const NamedList *nl, FILE *file);

KL_iter named_list_iter(NamedList *nl);

#endif /* ENTITY_NAMED_LIST_NAMED_LIST_H_ */