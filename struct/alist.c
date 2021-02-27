// alist.c
//
// Created on: Jan 14, 2018
//     Author: Jeff Manzione

#include "alist.h"

#include <string.h>

#include "alloc/alloc.h"
#include "debug/debug.h"
#include "util/util.h"

inline AList *__alist_create(void *arr, size_t obj_sz, size_t table_sz) {
  ASSERT(NOT_NULL(arr));
  AList *a = ALLOC(AList);
  __alist_init(a, arr, obj_sz, table_sz);
  return a;
}

inline void __alist_init(AList *e, void *arr, size_t obj_sz, size_t table_sz) {
  ASSERT(NOT_NULL(e), NOT_NULL(arr));
  e->_arr = (char *)arr;
  e->_len = 0;
  e->_obj_sz = obj_sz;
  e->_table_sz = table_sz;
}

inline size_t alist_append(AList *const e, const void *v) {
  ASSERT(NOT_NULL(e), NOT_NULL(v));
  if (e->_len == e->_table_sz) {
    e->_table_sz += DEFAULT_ARRAY_SZ;
    e->_arr = REALLOC_SZ(e->_arr, e->_obj_sz, e->_table_sz);
  }
  char *start = e->_arr + (e->_len * e->_obj_sz);
  memmove(start, v, e->_obj_sz);
  return e->_len++;
}

inline void *alist_add(AList *const e) {
  ASSERT(NOT_NULL(e));
  if (e->_len == e->_table_sz) {
    e->_table_sz += DEFAULT_ARRAY_SZ;
    e->_arr = REALLOC_SZ(e->_arr, e->_obj_sz, e->_table_sz);
  }
  return e->_arr + (e->_len++ * e->_obj_sz);
}

inline void alist_remove_last(AList *const e) {
  ASSERT(NOT_NULL(e));
  if (e->_len <= 0)
    return;
  e->_len--;
}

inline void alist_finalize(AList *const e) {
  ASSERT(NOT_NULL(e), NOT_NULL(e->_arr));
  DEALLOC(e->_arr);
}

inline void alist_delete(AList *const e) {
  ASSERT(NOT_NULL(e), NOT_NULL(e->_arr));
  alist_finalize(e);
  DEALLOC(e);
}

inline void *alist_get(const AList *const e, uint32_t i) {
  ASSERT(NOT_NULL(e), NOT_NULL(e->_arr), i >= 0, i < e->_len);
  return e->_arr + (i * e->_obj_sz);
}

inline size_t alist_len(const AList *const e) {
  ASSERT(NOT_NULL(e), NOT_NULL(e->_arr));
  return e->_len;
}

inline void alist_iterate(const AList *const e, EAction action) {
  int i;
  for (i = 0; i < alist_len(e); ++i) {
    action(alist_get(e, i));
  }
}

inline AL_iter alist_iter(const AList *const a) {
  AL_iter iter = {._list = a, ._i = 0};
  return iter;
}

inline void *al_value(AL_iter *iter) {
  return alist_get(iter->_list, iter->_i);
}

inline void al_inc(AL_iter *iter) { ++iter->_i; }

inline bool al_has(AL_iter *iter) { return iter->_i < alist_len(iter->_list); }
