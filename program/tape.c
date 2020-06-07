// instruction.h
//
// Created on: Jun 6, 2020
//     Author: Jeff Manzione

#include "program/tape.h"

#include "alloc/alloc.h"
#include "debug/debug.h"
#include "lang/lexer/token.h"
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
  AList supers;
} _ClassRef;

struct _Tape {
  const char *module_name;
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
  alist_init(&tape_ins, Instruction, DEFAULT_TAPE_SZ);
  alist_init(&tape_source_map, SourceMapping, DEFAULT_TAPE_SZ);
  keyedlist_init(&tape_class_refs, _ClassRef, DEFAULT_ARRAY_SZ);
  keyedlist_init(&tape_func_refs, _ClassRef, DEFAULT_ARRAY_SZ);
  tape_current_class = NULL;
  return tape;
}

void tape_delete(Tape *tape) {
  ASSERT(NOT_NULL(tape));
  alist_finalize(&tape_ins);
  alist_finalize(&tape_source_map);
  M_iter class_i = map_iter(&tape_class_refs._map);
  for (; has(&class_i); inc(&class_i)) {
    _classref_finalize((_ClassRef *)value(&class_i));
  }
  keyedlist_finalize(&tape_class_refs);
  keyedlist_finalize(&tape_func_refs);
  DEALLOC(tape);
}

Instruction *tape_add(Tape *tape) {
  SourceMapping *sm = (SourceMapping *)alist_add(&tape_source_map);
  // No sourcemap info present.
  sm->col = -1;
  sm->line = -1;
  return (Instruction *)alist_add(&tape_ins);
}

SourceMapping *tape_add_source(Tape *tape, Instruction *ins) {
  ASSERT(NOT_NULL(tape), NOT_NULL(ins));
  uintptr_t index = ins - (Instruction *)tape_ins._arr;
  ASSERT(index >= 0, index < tape_size(tape));
  return (SourceMapping *)alist_get(&tape_source_map, index);
}

inline void tape_start_func(Tape *tape, const char name[]) {
  ASSERT(NOT_NULL(tape), NOT_NULL(name));
  _FunctionRef *ref, *old;
  // In a class.
  if (NULL != tape_current_class) {
    old = (_FunctionRef *)keyedlist_insert(&tape_current_class->func_refs,
                                           name, (void **)&ref);
  } else {
    old =
        (_FunctionRef *)keyedlist_insert(&tape_func_refs, name, (void **)&ref);
  }
  if (NULL != old) {
    ERROR(
        "Attempting to add function %s, but a function by that name already "
        "exists.",
        name);
  }
  ref->name = name;
  ref->index = alist_len(&tape_ins);
}

void tape_start_class(Tape *tape, const char name[]) {
  ASSERT(NOT_NULL(tape), NOT_NULL(name));
  _ClassRef *ref;
  _ClassRef *old =
      (_ClassRef *)keyedlist_insert(&tape_class_refs, name, (void **)&ref);
  if (NULL != old) {
    ERROR(
        "Attempting to add class %s, but a function by that name already "
        "exists.",
        name);
  }
  _classref_init(ref, name);
  ref->start_index = alist_len(&tape_ins);
  tape_current_class = ref;
}

void tape_end_class(Tape *tape) {
  ASSERT(NOT_NULL(tape), NOT_NULL(tape_current_class));
  tape_current_class->end_index = alist_len(&tape_ins);
  tape_current_class = NULL;
}

inline const Instruction *tape_get(Tape *tape, uint32_t index) {
  ASSERT(NOT_NULL(tape), index >= 0, index < tape_size(tape));
  return (Instruction *)alist_get(&tape_ins, index);
}

inline const SourceMapping *tape_get_source(Tape *tape, uint32_t index) {
  ASSERT(NOT_NULL(tape), index >= 0, index < tape_size(tape));
  return (SourceMapping *)alist_get(&tape_source_map, index);
}

inline size_t tape_size(const Tape *tape) {
  ASSERT(NOT_NULL(tape));
  return alist_len(&tape_ins);
}

inline void _classref_init(_ClassRef *ref, const char name[]) {
  ref->name = name;
  keyedlist_init(&ref->func_refs, _FunctionRef, DEFAULT_ARRAY_SZ);
  alist_init(&ref->supers, char *, DEFAULT_ARRAY_SZ);
}

inline void _classref_finalize(_ClassRef *ref) {
  keyedlist_finalize(&ref->func_ref);
  alist_finalize(&ref->supers);
}

void tape_write(const Tape *tape, FILE *file) {
  ASSERT(NOT_NULL(tape), NOT_NULL(file));
  KL_iter cls_iter = keyedlist_iter((KeyedList *)&tape_class_refs);
  KL_iter func_iter = keyedlist_iter((KeyedList *)&tape_func_refs);
  KL_iter cls_func_iter;
  bool in_class = false;
  int i;
  for (i = 0; i <= alist_len(&tape_ins); ++i) {
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
    if (i < alist_len(&tape_ins)) {
      Instruction *ins = alist_get(&tape_ins, i);
      instruction_write(ins, file);
      SourceMapping *sm = (SourceMapping *)alist_get(&tape_source_map, i);
      if (sm->col >= 0 && sm->line >= 0) {
        fprintf(file, "  ; line=%d, col=%d", sm->line, sm->col);
      }
      fprintf(file, "\n");
    }
  }
}

// Consumes tail.
void tape_append(Tape *head, Tape *tail) {
  ASSERT(NOT_NULL(head), NOT_NULL(tail));

  int previous_head_length = alist_len(&head->ins);

  // Copy all instructions from tail to head.
  int i;
  for (i = 0; i < alist_len(&tail->ins); ++i) {
    Instruction *cpy = tape_add(head);
    SourceMapping *sm_cpy = tape_add_source(cpy);
    *cpy = *((Instruction *)alist_get(&tail->ins, i));
    *sm_cpy = *tape_get_source(tail, i);
  }
  // Copy all functions.
  KL_iter func_iter = keyedlist_iter(&tail->func_refs);
  for (; kl_has(&func_iter); kl_inc(&func_iter)) {
    _FunctionRef *old_func = (_FunctionRef *)kl_value(&func_iter);
    _FunctionRef *cpy =
        (_FunctionRef *)keyedlist_insert(&head->func_refs, old_func->name);
    *cpy = *old_func;
    cpy->index += previous_head_length;
  }
  // Copy all classes.
  KL_iter class_iter = keyedlist_iter(&tail->class_refs);
  for (; kl_has(&class_iter); kl_inc(&class_iter)) {
    _ClassRef *old_class = (_ClassRef *)kl_value(&class_iter);
    _ClassRef *cpy =
        (_ClassRef *)keyedlist_insert(&head->class_refs, old_class->name);
    *cpy = *old_class;
    cpy->start_index += previous_head_length;
    cpy->end_index += previous_head_length;
    // Copy all methods.
    KL_iter func_iter = keyedlist_iter(&old_class->func_refs);
    for (; kl_has(&func_iter); kl_inc(&func_iter)) {
      _FunctionRef *old_func = (_FunctionRef *)kl_value(&func_iter);
      _FunctionRef *cpy =
          (_FunctionRef *)keyedlist_insert(&cpy->func_refs, old_func->name);
      *cpy = *old_func;
      cpy->index += previous_head_length;
    }
  }
  // Dealloc all of tail.
  alist_finalize(&tail->ins);
  alist_finalize(&tail->source_map);
  keyedlist_finalize(&tape_class_refs);
  keyedlist_finalize(&tape_func_refs);
  DEALLOC(tape);
}

// **********************
// Specialized functions.
// **********************

int tape_ins(Tape *tape, Op op, const Token *token) {
  ASSERT(NOT_NULL(tape), NOT_NULL(token));
  Instruction *ins = tape_add(tape);
  SourceMapping *sm = tape_add_source(tape, ins);

  ins->op = op;
  sm->token = token;
  sm->line = token->line;
  sm->col = token->col;

  switch (token->type) {
    case INTEGER:
    case FLOATING:
      ins->type = INSTRUCTION_PRIMITIVE;
      ins->val = token_to_primitive(token);
      break;
    case STR:
      ins->type = INSTRUCTION_STRING;
      ins->str = token->text;
      break;
    case WORD:
    default:
      ins->tpe = INSTRUCTION_ID;
      ins->idtoken->text;
      break;
  }
  return 1;
}

int tape_ins_neg(Tape *tape, Op op, const Token *token) {
  ASSERT(NOT_NULL(tape), NOT_NULL(token));
  Instruction *ins = tape_add(tape);
  SourceMapping *sm = tape_add_source(tape, ins);

  ins->op = op;
  sm->token = token;
  sm->line = token->line;
  sm->col = token->col;

  ins->type = INSTRUCTION_PRIMITIVE;
  ins->val = token_to_primitive(token);
  return 1;
}

int tape_ins_text(Tape *tape, Op op, const char text[], const Token *token) {
  ASSERT(NOT_NULL(tape), NOT_NULL(token));
  Instruction *ins = tape_add(tape);
  SourceMapping *sm = tape_add_source(tape, ins);

  ins->op = op;
  sm->token = token;
  sm->line = token->line;
  sm->col = token->col;

  ins->type = INSTRUCTION_ID;
  ins->id = text;
  return 1;
}

int tape_ins_int(Tape *tape, Op op, int val, const Token *token) {
  ASSERT(NOT_NULL(tape), NOT_NULL(token));
  Instruction *ins = tape_add(tape);
  SourceMapping *sm = tape_add_source(tape, ins);

  ins->op = op;
  sm->token = token;
  sm->line = token->line;
  sm->col = token->col;

  ins->type = INSTRUCTION_PRIMITIVE;
  ins->val = {._type = INT, ._int_val = val};
  return 1;
}

int tape_ins_no_arg(Tape *tape, Op op, const Token *token) {
  ASSERT(NOT_NULL(tape), NOT_NULL(token));
  Instruction *ins = tape_add(tape);
  SourceMapping *sm = tape_add_source(tape, ins);

  ins->op = op;
  sm->token = token;
  sm->line = token->line;
  sm->col = token->col;

  ins->type = INSTRUCTION_NO_ARG;
  return 1;
}

// void insert_label_index(Tape *const tape, const char key[], int index) {
//   if (queue_size(&tape_class_prefs) < 1) {
//     map_insert(&tape_refs, key, (void *)index);
//     return;
//   }
//   Map *methods = map_lookup(&tape_classes, queue_peek(&tape_class_prefs));
//   map_insert(methods, key, (void *)index);
// }

// void insert_label(Tape *const tape, const char key[]) {
//   insert_label_index(tape, key, expando_len(tape_instructions));
// }

int tape_label(Tape *tape, const Token *token) {
  tape_start_func(tape, token->text);
  return 0;
}

int tape_label_text(Tape *tape, const char text[]) {
  tape_start_func(tape, text);
  return 0;
}

char *anon_fn_for_token(const Token *token) {
  size_t needed = snprintf(NULL, 0, "$anon_%d_%d", token->line, token->col) + 1;
  char *buffer = ALLOC_ARRAY2(char, needed);
  snprintf(buffer, needed, "$anon_%d_%d", token->line, token->col);
  char *label = strings_intern(buffer);
  DEALLOC(buffer);
  return label;
}

int tape_anon_label(Tape *tape, const Token *token) {
  char *label = anon_fn_for_token(token);
  tape_label_text(tape, label);
  return 0;
}

int tape_ins_anon(Tape *tape, Op op, const Token *token) {
  ASSERT(NOT_NULL(tape), NOT_NULL(token));
  Instruction *ins = tape_add(tape);
  SourceMapping *sm = tape_add_source(tape, ins);

  ins->op = op;
  sm->token = token;
  sm->line = token->line;
  sm->col = token->col;

  ins->type = INSTRUCTION_ID;
  ins->id = anon_fn_for_token(token);
  return 1;
}

int tape_module(Tape *tape, const Token *token) {
  tape_module_name = token->text;
  return 0;
}

int tape_class(Tape *tape, const Token *token) {
  tape_start_class(tape, token->text);
  return 0;
}

int tape_class_with_parents(Tape *tape, const Token *token, Q *q_parents) {
  tape_class(tape, token);
  _ClassRef *cls = tape_current_class;
  while (Q_size(q_parents) > 0) {
    char *parent = queue_remove(q_parents);
    alist_append(parents, &parent);
  }
  return 0;
}

int tape_endclass(Tape *tape, const Token *token) {
  tape_end_class(tape);
  return 0;
}