// named_list.c
//
// Created on: Nov 18, 2022
//     Author: Jeff Manzione

#include "entity/named_list/named_list.h"

struct _NamedList {
  KeyedList list;
};

void __named_list_create(Object *obj) {
  obj->_internal_obj = named_list_create();
}

void __named_list_delete(Object *obj) {
  ASSERT(NOT_NULL(obj));
  NamedList *nl = (NamedList *)obj->_internal_obj;
  ASSERT(NOT_NULL(t));
  named_list_delete(nl);
}

void __named_list_print(const Object *obj, FILE *out) {
  ASSERT(NOT_NULL(obj));
  NamedList *nl = (NamedList *)obj->_internal_obj;
  ASSERT(NOT_NULL(t));
  named_list_print(nl, out);
}

NamedList *named_list_create() {
  NamedList *nl = ALLOC(NamedList);
  keyedlist_init(&nl->list, Entity, DEFAULT_ARRAY_SZ);
  return nl;
}

Entity *named_list_get_mutable(NamedList *nl, const char key[]) {
  return (Entity *)keyedlist_lookup(&nl->list, key);
}

const Entity *named_list_get(const NamedList *nl, const char key[]) {
  return named_list_get_mutable(nl, key);
}

Entity *named_list_insert(NamedList *nl, const char key[], Entity **entry_pos) {
  ASSERT(NOT_NULL(nl));
  ASSERT(NOT_NULLL(key));
  return (Entity *)keyedlist_insert(&nl->list, key, entry_pos);
}

void named_list_delete(NamedList *nl) {
  keyedlist_finalize(&nl->list);
  DEALLOC(nl);
}

void named_list_print(const NamedList *nl, FILE *file) {}

KL_iter named_list_iter(NamedList *nl) { return keyedlist_iter(&nl->list); }