// instruction.h
//
// Created on: Jun 6, 2020
//     Author: Jeff Manzione

#include "program/tape.h"

#include "alloc/alloc.h"
#include "alloc/arena/intern.h"
#include "debug/debug.h"
#include "lang/lexer/token.h"
#include "program/instruction.h"
#include "struct/alist.h"
#include "struct/keyed_list.h"
#include "struct/q.h"

#define DEFAULT_TAPE_SZ 64

#define CLASS_KEYWORD "class"
#define CLASSEND_KEYWORD "endclass"
#define MODULE_KEYWORD "module"
#define INSTRUCTION_COMMENT_LPAD 16
#define PADDING ""

struct _Tape {
  const char *module_name;
  AList ins;
  AList source_map;
  KeyedList class_refs;
  KeyedList func_refs;

  ClassRef *current_class;
};

void _classref_init(ClassRef *ref, const char name[]);
void _classref_finalize(ClassRef *ref);

inline Tape *tape_create() {
  Tape *tape = ALLOC2(Tape);
  alist_init(&tape->ins, Instruction, DEFAULT_TAPE_SZ);
  alist_init(&tape->source_map, SourceMapping, DEFAULT_TAPE_SZ);
  keyedlist_init(&tape->class_refs, ClassRef, DEFAULT_ARRAY_SZ);
  keyedlist_init(&tape->func_refs, ClassRef, DEFAULT_ARRAY_SZ);
  tape->current_class = NULL;
  return tape;
}

void tape_delete(Tape *tape) {
  ASSERT(NOT_NULL(tape));
  alist_finalize(&tape->ins);
  alist_finalize(&tape->source_map);
  KL_iter class_i = keyedlist_iter(&tape->class_refs);
  for (; kl_has(&class_i); kl_inc(&class_i)) {
    _classref_finalize((ClassRef *)kl_value(&class_i));
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
  FunctionRef *ref, *old;
  // In a class.
  if (NULL != tape->current_class) {
    old = (FunctionRef *)keyedlist_insert(&tape->current_class->func_refs, name,
                                          (void **)&ref);
  } else {
    old =
        (FunctionRef *)keyedlist_insert(&tape->func_refs, name, (void **)&ref);
  }
  if (NULL != old) {
    ERROR("Attempting to add function %s, but a function by that name already "
          "exists.",
          name);
  }
  ref->name = name;
  ref->index = alist_len(&tape->ins);
}

void tape_start_class(Tape *tape, const char name[]) {
  ASSERT(NOT_NULL(tape), NOT_NULL(name));
  ClassRef *ref;
  ClassRef *old =
      (ClassRef *)keyedlist_insert(&tape->class_refs, name, (void **)&ref);
  if (NULL != old) {
    ERROR("Attempting to add class %s, but a function by that name already "
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

inline const Instruction *tape_get(const Tape *tape, uint32_t index) {
  ASSERT(NOT_NULL(tape), index >= 0, index < tape_size(tape));
  return (Instruction *)alist_get(&tape->ins, index);
}

inline Instruction *tape_get_mutable(Tape *tape, uint32_t index) {
  ASSERT(NOT_NULL(tape), index >= 0, index < tape_size(tape));
  return (Instruction *)alist_get(&tape->ins, index);
}

inline const SourceMapping *tape_get_source(const Tape *tape, uint32_t index) {
  ASSERT(NOT_NULL(tape), index >= 0, index < tape_size(tape));
  return (SourceMapping *)alist_get(&tape->source_map, index);
}

inline size_t tape_size(const Tape *tape) {
  ASSERT(NOT_NULL(tape));
  return alist_len(&tape->ins);
}

inline KL_iter tape_classes(const Tape *tape) {
  return keyedlist_iter((KeyedList *)&tape->class_refs); // bless
}

inline KL_iter tape_functions(const Tape *tape) {
  return keyedlist_iter((KeyedList *)&tape->func_refs); // bless
}

inline const char *tape_module_name(const Tape *tape) {
  return tape->module_name;
}

inline void _classref_init(ClassRef *ref, const char name[]) {
  ref->name = name;
  keyedlist_init(&ref->func_refs, FunctionRef, DEFAULT_ARRAY_SZ);
  alist_init(&ref->supers, char *, DEFAULT_ARRAY_SZ);
}

inline void _classref_finalize(ClassRef *ref) {
  keyedlist_finalize(&ref->func_refs);
  alist_finalize(&ref->supers);
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
      ClassRef *class_ref = (ClassRef *)kl_value(&cls_iter);
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
      FunctionRef *func_ref = (FunctionRef *)kl_value(&cls_func_iter);
      if (func_ref->index == i) {
        fprintf(file, "@%s\n", func_ref->name);
        kl_inc(&cls_func_iter);
      }
    } else if (kl_has(&func_iter)) {
      FunctionRef *func_ref = (FunctionRef *)kl_value(&func_iter);
      if (func_ref->index == i) {
        fprintf(file, "@%s\n", func_ref->name);
        kl_inc(&func_iter);
      }
    }
    if (i < alist_len(&tape->ins)) {
      Instruction *ins = alist_get(&tape->ins, i);
      int chars_written = instruction_write(ins, file);
      SourceMapping *sm = (SourceMapping *)alist_get(&tape->source_map, i);
      if (sm->col >= 0 && sm->line >= 0) {
        int lpadding = max(INSTRUCTION_COMMENT_LPAD - chars_written, 0);
        fprintf(file, "%*s; line=%d, col=%d", lpadding, PADDING, sm->line,
                sm->col);
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
    SourceMapping *sm_cpy = tape_add_source(head, cpy);
    *cpy = *((Instruction *)alist_get(&tail->ins, i));
    *sm_cpy = *tape_get_source(tail, i);
  }
  // Copy all functions.
  KL_iter func_iter = keyedlist_iter(&tail->func_refs);
  for (; kl_has(&func_iter); kl_inc(&func_iter)) {
    FunctionRef *old_func = (FunctionRef *)kl_value(&func_iter);
    FunctionRef *cpy;
    keyedlist_insert(&head->func_refs, old_func->name, (void **)&cpy);
    *cpy = *old_func;
    cpy->index += previous_head_length;
  }
  // Copy all classes.
  KL_iter class_iter = keyedlist_iter(&tail->class_refs);
  for (; kl_has(&class_iter); kl_inc(&class_iter)) {
    ClassRef *old_class = (ClassRef *)kl_value(&class_iter);
    ClassRef *cpy_class;
    keyedlist_insert(&head->class_refs, old_class->name, (void **)&cpy_class);
    *cpy_class = *old_class;
    cpy_class->start_index += previous_head_length;
    cpy_class->end_index += previous_head_length;
    // Copy all methods.
    KL_iter func_iter = keyedlist_iter(&old_class->func_refs);
    for (; kl_has(&func_iter); kl_inc(&func_iter)) {
      FunctionRef *old_func = (FunctionRef *)kl_value(&func_iter);
      FunctionRef *cpy;
      keyedlist_insert(&cpy_class->func_refs, old_func->name, (void **)&cpy);
      *cpy = *old_func;
      cpy->index += previous_head_length;
    }
  }
  // Dealloc all of tail.
  alist_finalize(&tail->ins);
  alist_finalize(&tail->source_map);
  keyedlist_finalize(&tail->class_refs);
  keyedlist_finalize(&tail->func_refs);
  DEALLOC(tail);
}

Token *_q_peek(Q *tokens) {
  if (Q_size(tokens) <= 0) {
    return NULL;
  }
  return Q_get(tokens, 0);
}

Token *_next_token_skip_ln(Q *queue) {
  ASSERT_NOT_NULL(queue);
  ASSERT(Q_size(queue) > 0);
  Token *first = (Token *)Q_remove(queue, 0);
  ASSERT_NOT_NULL(first);
  while (first->type == ENDLINE) {
    first = (Token *)_q_peek(queue);
    if (NULL == first) {
      return NULL;
    }
    Q_remove(queue, 0);
  }
  return first;
}

void tape_read_ins(Tape *const tape, Q *tokens) {
  ASSERT_NOT_NULL(tokens);
  if (Q_size(tokens) < 1) {
    return;
  }
  Token *first = _next_token_skip_ln(tokens);
  if (NULL == first) {
    return;
  }
  if (AT == first->type) {
    Token *fn_name = Q_remove(tokens, 0);
    if (ENDLINE != ((Token *)_q_peek(tokens))->type) {
      ERROR("Invalid token after @def.");
    }
    tape_label(tape, fn_name);
    return;
  }
  if (0 == strcmp(CLASS_KEYWORD, first->text)) {
    Token *class_name = Q_remove(tokens, 0);
    if (ENDLINE == ((Token *)_q_peek(tokens))->type) {
      tape_class(tape, class_name);
      return;
    }
    Q parents;
    Q_init(&parents);
    while (COMMA == ((Token *)_q_peek(tokens))->type) {
      Q_remove(tokens, 0);
      *Q_add_last(&parents) = (char *)((Token *)Q_remove(tokens, 0))->text;
    }
    tape_class_with_parents(tape, class_name, &parents);
    Q_finalize(&parents);
    return;
  }
  if (0 == strcmp(CLASSEND_KEYWORD, first->text)) {
    tape_endclass(tape, first);
    return;
  }
  Op op = str_to_op(first->text);
  Token *next = (Token *)_q_peek(tokens);
  if (ENDLINE == next->type || POUND == next->type) {
    tape_ins_no_arg(tape, op, first);
  } else if (MINUS == next->type) {
    Q_remove(tokens, 0);
    tape_ins_neg(tape, op, Q_remove(tokens, 0));
  } else {
    Q_remove(tokens, 0);
    tape_ins(tape, op, next);
  }
}

void tape_read(Tape *const tape, Q *tokens) {
  ASSERT(NOT_NULL(tape), NOT_NULL(tokens));
  if (0 == strcmp(MODULE_KEYWORD, ((Token *)_q_peek(tokens))->text)) {
    Q_remove(tokens, 0);
    Token *module_name = (Token *)Q_remove(tokens, 0);
    tape->module_name = module_name->text;
  } else {
    tape->module_name = intern("$");
  }
  while (Q_size(tokens) > 0) {
    tape_read_ins(tape, tokens);
  }
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
    ins->type = INSTRUCTION_ID;
    ins->id = token->text;
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
  Primitive p = token_to_primitive(token);
  ins->val = primitive_int(-pint(&p));
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

DEB_FN(int, tape_ins_int, Tape *tape, Op op, int val, const Token *token) {
  ASSERT(NOT_NULL(tape), NOT_NULL(token));
  Instruction *ins = tape_add(tape);
  SourceMapping *sm = tape_add_source(tape, ins);

  ins->op = op;
  sm->token = token;
  sm->line = token->line;
  sm->col = token->col;

  ins->type = INSTRUCTION_PRIMITIVE;
  ins->val._type = INT;
  ins->val._int_val = val;
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
  char *label = intern(buffer);
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
  tape->module_name = token->text;
  return 0;
}

int tape_class(Tape *tape, const Token *token) {
  tape_start_class(tape, token->text);
  return 0;
}

int tape_class_with_parents(Tape *tape, const Token *token, Q *q_parents) {
  tape_class(tape, token);
  ClassRef *cls = tape->current_class;
  while (Q_size(q_parents) > 0) {
    char *parent = Q_dequeue(q_parents);
    alist_append(&cls->supers, &parent);
  }
  return 0;
}

int tape_endclass(Tape *tape, const Token *token) {
  tape_end_class(tape);
  return 0;
}