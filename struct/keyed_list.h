#ifndef STRUCT_KEYED_LIST_H_
#define STRUCT_KEYED_LIST_H_
// keyed_list.h
//
// Created on: Jun 03, 2020
//     Author: Jeff Manzione

#include "struct/alist.h"
#include "struct/map.h"

#define keyedlist_init(klist, type, table_sz) \
  __keyedlist_init((klist), #type, sizeof(type), (table_sz))

typedef struct {
  AList _list;
  Map _map;
} KeyedList;

void __keyedlist_init(KeyedList *klist, const char type_name[], size_t type_sz,
                      size_t table_sz);
void keyedlist_finalize(KeyedList *klist);
void *keyedlist_insert(KeyedList *klist, const void *key, void **entry);
void *keyedlist_lookup(KeyedList *klist, const void *key);

typedef struct {
  M_iter _iter;
  AList *_list;
} KL_iter;

KL_iter keyedlist_iter(KeyedList *klist);
bool kl_has(KL_iter *iter);
void kl_inc(KL_iter *iter);

const void *kl_key(KL_iter *iter);
const void *kl_value(KL_iter *iter);

#endif /* STRUCT_KEYED_LIST_H_ */