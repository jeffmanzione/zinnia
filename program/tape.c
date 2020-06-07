// instruction.h
//
// Created on: Jun 6, 2020
//     Author: Jeff Manzione

#include "program/tape.h"

#include "alloc/alloc.h"
#include "debug/debug.h"
#include "program/instruction.h"
#include "struct/alist.h"
#include "struct/keyed_list.h"

#define DEFAULT_TAPE_SZ 64

typedef struct {
  const char *name;
  uint32_t index;
} _FunctionRef;

typedef struct {
  const char *name;
  uint32_t start_index;  // inclusive
  uint32_t end_index;    // exclusive
  KeyedList func_refs;
} _ClassRef;

struct _Tape {
  AList ins;
  AList source_map;
  KeyedList class_refs;
  KeyedList func_refs;

  _ClassRef *current_class;
};

void _classref_init(_ClassRef *ref, const char name[]);
void _classref_finalize(_ClassRef *ref);

inline Tape *tape_create() {
  Tape *tape = ALLOC2(Tape);
  alist_init(&tape->ins, Instruction, DEFAULT_TAPE_SZ);
  alist_init(&tape->source_map, SourceMapping, DEFAULT_TAPE_SZ);
  keyedlist_init(&tape->class_refs, _ClassRef, DEFAULT_ARRAY_SZ);
  keyedlist_init(&tape->func_refs, _ClassRef, DEFAULT_ARRAY_SZ);
  tape->current_class = NULL;
  return tape;
}

void tape_delete(Tape *tape) {
  ASSERT(NOT_NULL(tape));
  alist_finalize(&tape->ins);
  alist_finalize(&tape->source_map);
  M_iter class_i = map_iter(&tape->class_refs._map);
  for (; has(&class_i); inc(&class_i)) {
    _classref_finalize((_ClassRef *)value(&class_i));
  }
  keyedlist_finalize(&tape->class_refs);
  keyedlist_finalize(&tape->func_refs);
  DEALLOC(tape);
}

Instruction *tape_add(Tape *tape) {
  SourceMapping *sm = (SourceMapping *)alist_add(&tape->source_map);
  // No sourcemap info present.
  sm->col = -1;
  sm->line = -1;
  return (Instruction *)alist_add(&tape->ins);
}

SourceMapping *tape_add_source(Tape *tape, Instruction *ins) {
  ASSERT(NOT_NULL(tape), NOT_NULL(ins));
  uintptr_t index = ins - (Instruction *)tape->ins._arr;
  ASSERT(index >= 0, index < tape_size(tape));
  return (SourceMapping *)alist_get(&tape->source_map, index);
}

inline void tape_start_func(Tape *tape, const char name[]) {
  ASSERT(NOT_NULL(tape), NOT_NULL(name));
  _FunctionRef *ref, *old;
  // In a class.
  if (NULL != tape->current_class) {
    old = (_FunctionRef *)keyedlist_insert(&tape->current_class->func_refs,
                                           name, (void **)&ref);
  } else {
    old =
        (_FunctionRef *)keyedlist_insert(&tape->func_refs, name, (void **)&ref);
  }
  if (NULL != old) {
    ERROR(
        "Attempting to add function %s, but a function by that name already "
        "exists.",
        name);
  }
  ref->name = name;
  ref->index = alist_len(&tape->ins);
}

void tape_start_class(Tape *tape, const char name[]) {
  ASSERT(NOT_NULL(tape), NOT_NULL(name));
  _ClassRef *ref;
  _ClassRef *old =
      (_ClassRef *)keyedlist_insert(&tape->class_refs, name, (void **)&ref);
  if (NULL != old) {
    ERROR(
        "Attempting to add class %s, but a function by that name already "
        "exists.",
        name);
  }
  _classref_init(ref, name);
  ref->start_index = alist_len(&tape->ins);
  tape->current_class = ref;
}

void tape_end_class(Tape *tape) {
  ASSERT(NOT_NULL(tape), NOT_NULL(tape->current_class));
  tape->current_class->end_index = alist_len(&tape->ins);
  tape->current_class = NULL;
}

inline const Instruction *tape_get(Tape *tape, uint32_t index) {
  ASSERT(NOT_NULL(tape), index >= 0, index < tape_size(tape));
  return (Instruction *)alist_get(&tape->ins, index);
}

inline const SourceMapping *tape_get_source(Tape *tape, uint32_t index) {
  ASSERT(NOT_NULL(tape), index >= 0, index < tape_size(tape));
  return (SourceMapping *)alist_get(&tape->source_map, index);
}

inline size_t tape_size(const Tape *tape) {
  ASSERT(NOT_NULL(tape));
  return alist_len(&tape->ins);
}

inline void _classref_init(_ClassRef *ref, const char name[]) {
  ref->name = name;
  keyedlist_init(&ref->func_refs, _FunctionRef, DEFAULT_ARRAY_SZ);
}

inline void _classref_finalize(_ClassRef *ref) {
  keyedlist_finalize(&ref->func_refs);
}

void tape_write(const Tape *tape, FILE *file) {
  ASSERT(NOT_NULL(tape), NOT_NULL(file));
  KL_iter cls_iter = keyedlist_iter((KeyedList *)&tape->class_refs);
  KL_iter func_iter = keyedlist_iter((KeyedList *)&tape->func_refs);
  KL_iter cls_func_iter;
  bool in_class = false;
  int i;
  for (i = 0; i <= alist_len(&tape->ins); ++i) {
    if (kl_has(&cls_iter)) {
      _ClassRef *class_ref = (_ClassRef *)kl_value(&cls_iter);
      if (in_class && class_ref->end_index == i) {
        in_class = false;
        fprintf(file, "endclass  ; %s\n", class_ref->name);
      } else if (!in_class && class_ref->start_index == i) {
        in_class = true;
        cls_func_iter = keyedlist_iter((KeyedList *)&class_ref->func_refs);
        fprintf(file, "class %s\n", class_ref->name);
      }
    }
    if (in_class && kl_has(&cls_func_iter)) {
      _FunctionRef *func_ref = (_FunctionRef *)kl_value(&cls_func_iter);
      if (func_ref->index == i) {
        fprintf(file, "@%s\n", func_ref->name);
      }
    } else if (kl_has(&func_iter)) {
      _FunctionRef *func_ref = (_FunctionRef *)kl_value(&func_iter);
      if (func_ref->index == i) {
        fprintf(file, "@%s\n", func_ref->name);
      }
    }
    if (i < alist_len(&tape->ins)) {
      Instruction *ins = alist_get(&tape->ins, i);
      instruction_write(ins, file);
      SourceMapping *sm = (SourceMapping *)alist_get(&tape->source_map, i);
      if (sm->col >= 0 && sm->line >= 0) {
        fprintf(file, "  ; line=%d, col=%d", sm->line, sm->col);
      }
      fprintf(file, "\n");
    }
  }
}