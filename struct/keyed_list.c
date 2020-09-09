// keyed_list.h
//
// Created on: Jun 03, 2020
//     Author: Jeff Manzione

#include "struct/keyed_list.h"

#include "alloc/alloc.h"
#include "debug/debug.h"
#include "struct/alist.h"
#include "struct/map.h"
#include "struct/struct_defaults.h"

void __keyedlist_init(KeyedList *klist, const char type_name[], size_t type_sz,
                      size_t table_sz) {
  ASSERT(NOT_NULL(klist));
  __alist_init(&klist->_list, ALLOC_ARRAY_SZ(type_name, type_sz, table_sz),
               type_sz, table_sz);
  map_init_default(&klist->_map);
}

void keyedlist_finalize(KeyedList *klist) {
  ASSERT(NOT_NULL(klist));
  map_finalize(&klist->_map);
  alist_finalize(&klist->_list);
}

void *keyedlist_insert(KeyedList *klist, const void *key, void **entry) {
  ASSERT(NOT_NULL(klist), NOT_NULL(key));
  void *existing = map_lookup(&klist->_map, key);
  if (NULL == existing) {
    *entry = alist_add(&klist->_list);
    // Dear god, please forgive me...
    map_insert(&klist->_map, key,
               (void *)((char *)(*entry) - klist->_list._arr + 1));
    return NULL;
  }
  // And this one too.
  existing = (void *)(klist->_list._arr + (uintptr_t)existing - 1);
  *entry = existing;
  return existing;
}

void *keyedlist_lookup(KeyedList *klist, const void *key) {
  ASSERT(NOT_NULL(klist), NOT_NULL(key));
  void *lookup_val = map_lookup(&klist->_map, key);
  if (NULL == lookup_val) {
    return NULL;
  }
  return klist->_list._arr + (uintptr_t)lookup_val - 1;
}

inline KL_iter keyedlist_iter(KeyedList *klist) {
  ASSERT(NOT_NULL(klist));
  KL_iter iter = {._iter = map_iter(&klist->_map), ._list = &klist->_list};
  return iter;
}

inline bool kl_has(KL_iter *iter) { return has(&iter->_iter); }

inline void kl_inc(KL_iter *iter) { inc(&iter->_iter); }

inline const void *kl_key(KL_iter *iter) { return key(&iter->_iter); }

inline const void *kl_value(KL_iter *iter) {
  return (void *)(iter->_list->_arr + (uintptr_t)value(&iter->_iter) - 1);
}