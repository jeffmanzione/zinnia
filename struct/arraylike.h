// arraylike.h
//
// Created on: Sep 15, 2018
//     Author: Jeff Manzione

#ifndef STRUCT_ARRAYLIKE_H_
#define STRUCT_ARRAYLIKE_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "alloc/alloc.h"
#include "debug/debug.h"

#define DEFAULT_TABLE_SIZE 8

#ifndef max
#define max(a, b) ((a) >= (b) ? (a) : (b))
#endif

#define DEFINE_ARRAYLIKE(name, type)                                           \
  typedef struct name##_ name;                                                 \
  struct name##_ {                                                             \
    uint32_t table_size;                                                       \
    uint32_t num_elts;                                                         \
    type *table;                                                               \
  };                                                                           \
  typedef struct {                                                             \
    uint32_t _i;                                                               \
    name *_arr;                                                                \
  } name##_iter;                                                               \
  void name##_init_sz(name *, size_t table_sz);                                \
  void name##_init(name *);                                                    \
  name *name##_create();                                                       \
  name *name##_create_sz(size_t len);                                          \
  name *name##_create_copy(const type input[], size_t len);                    \
  void name##_finalize(name *);                                                \
  void name##_delete(name *);                                                  \
  void name##_clear(name *const);                                              \
  void name##_lshrink(name *const array, size_t amount);                       \
  void name##_rshrink(name *const array, size_t amount);                       \
  void name##_push(name *const, type);                                         \
  type name##_pop(name *const);                                                \
  void name##_enqueue(name *const, type);                                      \
  type name##_dequeue(name *const);                                            \
  type *name##_add_last(name *const);                                          \
  void name##_set(name *const, uint32_t, type);                                \
  type *name##_set_ref(name *const array, uint32_t index);                     \
  type name##_get(name *const, uint32_t);                                      \
  type name##_last(name *const);                                               \
  type *name##_get_ref(name *const, uint32_t);                                 \
  type name##_remove(name *const, uint32_t);                                   \
  uint32_t name##_size(const name *const);                                     \
  bool name##_is_empty(const name *const);                                     \
  name *name##_copy(const name *const);                                        \
  void name##_append(name *const head, const name *const tail);                \
  void name##_append_range(name *const head, const name *const tail,           \
                           uint32_t tail_range_start,                          \
                           uint32_t tail_range_end);                           \
  void name##_shift_amount(name *const array, uint32_t start_pos,              \
                           uint32_t count, int32_t amount);                    \
  name##_iter name##_iterator(name *const);                                    \
  bool name##_has(name##_iter *);                                              \
  void name##_inc(name##_iter *);                                              \
  type *name##_value(name##_iter *)

#define IMPL_ARRAYLIKE(name, type)                                             \
  inline void name##_init_sz(name *array, size_t table_sz) {                   \
    array->table = ALLOC_ARRAY(type, array->table_size = table_sz);            \
    array->num_elts = 0;                                                       \
  }                                                                            \
  inline void name##_init(name *array) {                                       \
    name##_init_sz(array, DEFAULT_TABLE_SIZE);                                 \
  }                                                                            \
  inline name *name##_create() {                                               \
    name *array = ALLOC2(name);                                                \
    name##_init(array);                                                        \
    return array;                                                              \
  }                                                                            \
  name *name##_create_sz(size_t len) {                                         \
    name *array = ALLOC2(name);                                                \
    name##_init_sz(array, len);                                                \
    return array;                                                              \
  }                                                                            \
  name *name##_create_copy(const type input[], size_t len) {                   \
    name *array = ALLOC2(name);                                                \
    name##_init_sz(array,                                                      \
                   ((len / DEFAULT_TABLE_SIZE) + 1) * DEFAULT_TABLE_SIZE);     \
    memmove(array->table, input, len * sizeof(type));                          \
    array->num_elts = len;                                                     \
    return array;                                                              \
  }                                                                            \
  inline void name##_finalize(name *array) {                                   \
    ASSERT(NOT_NULL(array));                                                   \
    DEALLOC(array->table);                                                     \
  }                                                                            \
                                                                               \
  inline void name##_delete(name *array) {                                     \
    ASSERT(NOT_NULL(array));                                                   \
    name##_finalize(array);                                                    \
    DEALLOC(array);                                                            \
  }                                                                            \
                                                                               \
  void name##_maybe_realloc(name *const array, uint32_t need_to_accomodate) {  \
    ASSERT(NOT_NULL(array));                                                   \
    if (need_to_accomodate <= 0) {                                             \
      return;                                                                  \
    }                                                                          \
    uint32_t size = max(array->num_elts,                                       \
                        max(need_to_accomodate,                                \
                            (need_to_accomodate / DEFAULT_TABLE_SIZE + 1) *    \
                                DEFAULT_TABLE_SIZE));                          \
    if (size <= 0 || size <= array->table_size) {                              \
      return;                                                                  \
    }                                                                          \
    array->table = REALLOC(array->table, type, size);                          \
    array->table_size = size;                                                  \
    memset(array->table + array->num_elts, 0x0,                                \
           (array->table_size - array->num_elts) * sizeof(type));              \
  }                                                                            \
                                                                               \
  void name##_shift_amount(name *const array, uint32_t start_pos,              \
                           uint32_t count, int32_t amount) {                   \
    ASSERT(NOT_NULL(array), start_pos >= 0, start_pos + amount >= 0);          \
    if (0 == amount) {                                                         \
      return;                                                                  \
    }                                                                          \
    int new_size = max(array->num_elts, start_pos + count + amount);           \
    name##_maybe_realloc(array, new_size);                                     \
    memmove(array->table + start_pos + amount, array->table + start_pos,       \
            count * sizeof(type));                                             \
    if (amount > 0) {                                                          \
      memset(array->table + start_pos, 0x0, amount * sizeof(type));            \
    } else {                                                                   \
      memset(array->table + start_pos + count + amount, 0x0,                   \
             -amount * sizeof(type));                                          \
    }                                                                          \
    array->num_elts = new_size;                                                \
  }                                                                            \
  void name##_shift(name *const array, uint32_t start_pos, int32_t amount) {   \
    ASSERT(NOT_NULL(array), start_pos >= 0, start_pos + amount >= 0);          \
    if (0 == amount) {                                                         \
      return;                                                                  \
    }                                                                          \
    name##_maybe_realloc(array, array->num_elts + amount);                     \
    memmove(array->table + start_pos + amount, array->table + start_pos,       \
            (array->num_elts - start_pos) * sizeof(type));                     \
    if (amount > 0) {                                                          \
      memset(array->table + start_pos, 0x0, amount * sizeof(type));            \
    }                                                                          \
  }                                                                            \
                                                                               \
  inline void name##_clear(name *const array) {                                \
    ASSERT(NOT_NULL(array));                                                   \
    name##_maybe_realloc(array, array->num_elts = 0);                          \
  }                                                                            \
                                                                               \
  void name##_push(name *const array, type elt) {                              \
    ASSERT(NOT_NULL(array));                                                   \
    if (array->num_elts > 0) {                                                 \
      name##_shift(array, /*start_pos=*/0, /*amount=*/1);                      \
    }                                                                          \
    array->table[0] = elt;                                                     \
    array->num_elts++;                                                         \
  }                                                                            \
                                                                               \
  type name##_pop(name *const array) {                                         \
    ASSERT(NOT_NULL(array), array->num_elts != 0);                             \
    type to_return = array->table[0];                                          \
    if (array->num_elts > 1) {                                                 \
      name##_shift(array, /*start_pos=*/1, /*amount=*/-1);                     \
    }                                                                          \
    array->num_elts--;                                                         \
    return to_return;                                                          \
  }                                                                            \
                                                                               \
  void name##_enqueue(name *const array, type elt) {                           \
    ASSERT(NOT_NULL(array));                                                   \
    name##_maybe_realloc(array, array->num_elts + 1);                          \
    array->table[array->num_elts++] = elt;                                     \
  }                                                                            \
                                                                               \
  type name##_dequeue(name *const array) {                                     \
    ASSERT(NOT_NULL(array), array->num_elts != 0);                             \
    type to_return = array->table[array->num_elts - 1];                        \
    array->num_elts--;                                                         \
    return to_return;                                                          \
  }                                                                            \
                                                                               \
  type *name##_add_last(name *const array) {                                   \
    ASSERT(NOT_NULL(array));                                                   \
    name##_maybe_realloc(array, array->num_elts + 1);                          \
    return array->table + array->num_elts++;                                   \
  }                                                                            \
                                                                               \
  void name##_lshrink(name *const array, size_t amount) {                      \
    ASSERT(NOT_NULL(array), array->num_elts >= amount);                        \
    name##_shift(array, amount, -amount);                                      \
    array->num_elts -= amount;                                                 \
  }                                                                            \
                                                                               \
  void name##_rshrink(name *const array, size_t amount) {                      \
    ASSERT(NOT_NULL(array), array->num_elts >= amount);                        \
    array->num_elts -= amount;                                                 \
  }                                                                            \
                                                                               \
  void name##_set(name *const array, uint32_t index, type elt) {               \
    ASSERT(NOT_NULL(array), index >= 0);                                       \
    if (index >= array->num_elts) {                                            \
      name##_maybe_realloc(array, index + 1);                                  \
      array->num_elts = index + 1;                                             \
    }                                                                          \
    array->table[index] = elt;                                                 \
  }                                                                            \
  inline type *name##_set_ref(name *const array, uint32_t index) {             \
    ASSERT(NOT_NULL(array), index >= 0);                                       \
    if (index >= array->num_elts) {                                            \
      name##_maybe_realloc(array, index + 1);                                  \
      array->num_elts = index + 1;                                             \
    }                                                                          \
    return &array->table[index];                                               \
  }                                                                            \
                                                                               \
  inline type name##_get(name *const array, uint32_t index) {                  \
    ASSERT(NOT_NULL(array), index >= 0, index < array->num_elts);              \
    return array->table[index];                                                \
  }                                                                            \
  inline type name##_last(name *const array) {                                 \
    ASSERT(NOT_NULL(array));                                                   \
    return name##_get(array, name##_size(array) - 1);                          \
  }                                                                            \
                                                                               \
  inline type *name##_get_ref(name *const array, uint32_t index) {             \
    ASSERT(NOT_NULL(array), index >= 0, index < array->num_elts);              \
    return &array->table[index];                                               \
  }                                                                            \
                                                                               \
  type name##_remove(name *const array, uint32_t index) {                      \
    ASSERT(NOT_NULL(array), index >= 0, index < array->num_elts);              \
    type to_return = array->table[index];                                      \
    if (array->num_elts > 1) {                                                 \
      name##_shift(array, /*start_pos=*/index + 1, /*amount=*/-1);             \
    }                                                                          \
    array->num_elts--;                                                         \
    return to_return;                                                          \
  }                                                                            \
                                                                               \
  inline uint32_t name##_size(const name *const array) {                       \
    ASSERT(NOT_NULL(array));                                                   \
    return array->num_elts;                                                    \
  }                                                                            \
                                                                               \
  inline bool name##_is_empty(const name *const array) {                       \
    ASSERT(NOT_NULL(array));                                                   \
    return array->num_elts == 0;                                               \
  }                                                                            \
                                                                               \
  name *name##_copy(const name *const array) {                                 \
    ASSERT(NOT_NULL(array));                                                   \
    name *copy = ALLOC2(name);                                                 \
    *copy = *array;                                                            \
    copy->table = ALLOC_ARRAY2(type, array->table_size);                       \
    memmove(copy->table, array->table, sizeof(type) * array->table_size);      \
    return copy;                                                               \
  }                                                                            \
                                                                               \
  void name##_append(name *const head, const name *const tail) {               \
    ASSERT(NOT_NULL(head), NOT_NULL(tail));                                    \
    name##_maybe_realloc(head, head->num_elts + tail->num_elts);               \
    memmove(head->table + head->num_elts, tail->table,                         \
            sizeof(type) * tail->num_elts);                                    \
    head->num_elts += tail->num_elts;                                          \
  }                                                                            \
                                                                               \
  void name##_append_range(name *const head, const name *const tail,           \
                           uint32_t tail_range_start,                          \
                           uint32_t tail_range_end) {                          \
    ASSERT(NOT_NULL(head), NOT_NULL(tail), tail_range_start >= 0,              \
           tail_range_end >= tail_range_start);                                \
    ASSERT(name##_size(tail) >= tail_range_end);                               \
    name##_maybe_realloc(head,                                                 \
                         head->num_elts + tail_range_end - tail_range_start);  \
    memmove(head->table + head->num_elts, tail->table + tail_range_start,      \
            sizeof(type) * (tail_range_end - tail_range_start));               \
    head->num_elts += (tail_range_end - tail_range_start);                     \
  }                                                                            \
  inline name##_iter name##_iterator(name *const array) {                      \
    ASSERT(NOT_NULL(array));                                                   \
    name##_iter iter = {._i = 0, ._arr = array};                               \
    return iter;                                                               \
  }                                                                            \
  inline bool name##_has(name##_iter *iter) {                                  \
    return iter->_i < iter->_arr->num_elts;                                    \
  }                                                                            \
  inline void name##_inc(name##_iter *iter) { iter->_i++; }                    \
  inline type *name##_value(name##_iter *iter) {                               \
    return name##_get_ref(iter->_arr, iter->_i);                               \
  }

#endif /* STRUCT_ARRAYLIKE_H_ */
