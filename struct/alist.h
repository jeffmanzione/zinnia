// alist.h
//
// Created on: Jan 14, 2018
//     Author: Jeff

#ifndef STRUCT_ALIST_H_
#define STRUCT_ALIST_H_

#include <stddef.h>
#include <stdint.h>

#include "alloc/alloc.h"
#include "util/util.h"

typedef void (*ESwapper)(void *, void *);
typedef void (*EAction)(void *);

#define DEFAULT_ARRAY_SZ 6

typedef struct {
  char *_arr;
  size_t _len, _table_sz, _obj_sz;
} AList;

#define alist_init(e, type, table_sz) \
  __alist_init(e, ALLOC_ARRAY(type, table_sz), sizeof(type), table_sz)
void __alist_init(AList *e, void *arr, size_t obj_sz, size_t table_sz);

#define alist_create(type, table_sz) \
  __alist_create(ALLOC_ARRAY(type, table_sz), sizeof(type), table_sz)
AList *__alist_create(void *arr, size_t obj_sz, size_t table_sz);

void alist_finalize(AList *const a);
void alist_delete(AList *const a);
size_t alist_append(AList *const a, const void *v);
void *alist_add(AList *const a);
void alist_remove_last(AList *const a);
void *alist_get(const AList *const a, uint32_t i);
size_t alist_len(const AList *const a);
void alist_sort(AList *const a, Comparator c, ESwapper eswap);
void alist_iterate(const AList *const a, EAction action);

typedef struct {
  const AList *_list;
  uint32_t _i;
} AL_iter;

AL_iter alist_iter(const AList *const a);
void *al_value(AL_iter *iter);
void al_inc(AL_iter *iter);
bool al_has(AL_iter *iter);

#endif /* STRUCT_ALIST_H_ */
