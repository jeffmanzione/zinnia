// instruction.h
//
// Created on: Jun 6, 2020
//     Author: Jeff Manzione

#include "program/tape.h"

#include "alloc/alloc.h"
#include "alloc/arena/intern.h"
#include "debug/debug.h"
#include "lang/lexer/lang_lexer.h"
#include "lang/lexer/token.h"
#include "program/instruction.h"
#include "struct/alist.h"
#include "struct/keyed_list.h"
#include "struct/q.h"
#include "util/string_util.h"

#define DEFAULT_TAPE_SZ 64

#define CLASS_KEYWORD "class"
#define CLASSEND_KEYWORD "endclass"
#define MODULE_KEYWORD "module"
#define FIELD_KEYWORD "field"
#define SOURCE_KEYWORD "source"
#define INSTRUCTION_COMMENT_LPAD 16
#define PADDING ""

struct _Tape {
  const char *module_name;
  AList ins;

  AList source_map;
  char *external_source_fn;

  KeyedList class_refs;
  KeyedList func_refs;

  ClassRef *current_class;
};

void _classref_init(ClassRef *ref, const char name[]);
void _classref_finalize(ClassRef *ref);

Tape *tape_create() {
  Tape *tape = ALLOC2(Tape);
  alist_init(&tape->ins, Instruction, DEFAULT_TAPE_SZ);
  alist_init(&tape->source_map, SourceMapping, DEFAULT_TAPE_SZ);
  keyedlist_init(&tape->class_refs, ClassRef, DEFAULT_ARRAY_SZ);
  keyedlist_init(&tape->func_refs, FunctionRef, DEFAULT_ARRAY_SZ);
  tape->current_class = NULL;
  tape->external_source_fn = NULL;
  tape->module_name = NULL;
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

void tape_set_external_source(Tape *const tape, const char file_name[]) {
  ASSERT(NOT_NULL(tape));
  // ASSERT(NOT_NULL(file_name));
  tape->external_source_fn = (char *)file_name;
}

const char *tape_get_external_source(const Tape *const tape) {
  return tape->external_source_fn;
}

void tape_start_func_at_index(Tape *tape, const char name[], uint32_t index,
                              bool is_async) {
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
    FATALF("Attempting to add function %s, but a function by that name already "
           "exists.",
           name);
  }
  ref->name = name;
  ref->index = index;
  // TODO: Implement const functions.
  ref->is_const = false;
  ref->is_async = is_async;
}

void _tape_start_func(Tape *tape, const char name[], bool is_async) {
  tape_start_func_at_index(tape, name, alist_len(&tape->ins), is_async);
}

ClassRef *tape_start_class_at_index(Tape *tape, const char name[],
                                    uint32_t index) {
  ASSERT(NOT_NULL(tape), NOT_NULL(name));
  ClassRef *ref;
  ClassRef *old =
      (ClassRef *)keyedlist_insert(&tape->class_refs, name, (void **)&ref);
  if (NULL != old) {
    FATALF("Attempting to add class %s, but a function by that name already "
           "exists.",
           name);
  }
  _classref_init(ref, name);
  ref->start_index = index;
  tape->current_class = ref;
  return ref;
}

void tape_end_class_at_index(Tape *tape, uint32_t index) {
  ASSERT(NOT_NULL(tape), NOT_NULL(tape->current_class));
  tape->current_class->end_index = index;
  tape->current_class = NULL;
}

void tape_start_class(Tape *tape, const char name[]) {
  tape_start_class_at_index(tape, name, alist_len(&tape->ins));
}

void tape_end_class(Tape *tape) {
  tape_end_class_at_index(tape, alist_len(&tape->ins));
}

void _field_ref_init(FieldRef *fref, const char name[]) { fref->name = name; }

void tape_field(Tape *tape, const char *field) {
  ClassRef *cls = tape->current_class;
  if (NULL == cls) {
    FATALF("Cannot have field outside of a class.");
  }
  FieldRef *ref;
  FieldRef *old =
      (FieldRef *)keyedlist_insert(&cls->field_refs, field, (void **)&ref);
  if (NULL != old) {
    FATALF("Attempting to add field '%s', but a field by that name already "
           "exists.",
           field);
  }
  _field_ref_init(ref, field);
}

const Instruction *tape_get(const Tape *tape, uint32_t index) {
  ASSERT(NOT_NULL(tape), index >= 0, index < tape_size(tape));
  return (Instruction *)alist_get(&tape->ins, index);
}

Instruction *tape_get_mutable(Tape *tape, uint32_t index) {
  ASSERT(NOT_NULL(tape), index >= 0, index < tape_size(tape));
  return (Instruction *)alist_get(&tape->ins, index);
}

const SourceMapping *tape_get_source(const Tape *tape, uint32_t index) {
  ASSERT(NOT_NULL(tape), index >= 0, index < tape_size(tape));
  return (SourceMapping *)alist_get(&tape->source_map, index);
}

size_t tape_size(const Tape *tape) {
  ASSERT(NOT_NULL(tape));
  return alist_len(&tape->ins);
}

uint32_t tape_class_count(const Tape *tape) {
  return alist_len(&tape->class_refs._list);
}

KL_iter tape_classes(const Tape *tape) {
  return keyedlist_iter((KeyedList *)&tape->class_refs); // bless
}

const ClassRef *tape_get_class(const Tape *tape, const char class_name[]) {
  return keyedlist_lookup((KeyedList *)&tape->class_refs, class_name);
}

uint32_t tape_func_count(const Tape *tape) {
  return alist_len(&tape->func_refs._list);
}

KL_iter tape_functions(const Tape *tape) {
  return keyedlist_iter((KeyedList *)&tape->func_refs); // bless
}

const char *tape_module_name(const Tape *tape) { return tape->module_name; }

void _classref_init(ClassRef *ref, const char name[]) {
  ref->name = name;
  keyedlist_init(&ref->func_refs, FunctionRef, DEFAULT_ARRAY_SZ);
  keyedlist_init(&ref->field_refs, FieldRef, DEFAULT_ARRAY_SZ);
  alist_init(&ref->supers, char *, DEFAULT_ARRAY_SZ);
}

void _classref_finalize(ClassRef *ref) {
  keyedlist_finalize(&ref->func_refs);
  keyedlist_finalize(&ref->field_refs);
  alist_finalize(&ref->supers);
}

void tape_write(const Tape *tape, FILE *file) {
  ASSERT(NOT_NULL(tape), NOT_NULL(file));
  if (tape->module_name && 0 != strcmp("$", tape->module_name)) {
    fprintf(file, "module %s\n", tape->module_name);
  }
  if (NULL != tape->external_source_fn) {
    fprintf(file, "source '%s'\n", tape->external_source_fn);
  }
  AL_iter cls_iter = alist_iter((AList *)&tape->class_refs._list);
  AL_iter func_iter = alist_iter((AList *)&tape->func_refs._list);
  AL_iter cls_func_iter;
  bool in_class = false;
  int i;
  for (i = 0; i <= alist_len(&tape->ins); ++i) {
    if (al_has(&cls_iter)) {
      ClassRef *class_ref = (ClassRef *)al_value(&cls_iter);
      if (in_class && class_ref->end_index == i) {
        in_class = false;
        fprintf(file, "endclass  ; %s\n", class_ref->name);
        al_inc(&cls_iter);
      }
    }
    if (al_has(&cls_iter)) {
      ClassRef *class_ref = (ClassRef *)al_value(&cls_iter);
      if (!in_class && class_ref->start_index == i) {
        in_class = true;
        cls_func_iter = alist_iter((AList *)&class_ref->func_refs._list);
        fprintf(file, "class %s\n", class_ref->name);

        KL_iter fields = keyedlist_iter(&class_ref->field_refs);
        for (; kl_has(&fields); kl_inc(&fields)) {
          FieldRef *field = (FieldRef *)kl_value(&fields);
          fprintf(file, " field %s\n", field->name);
        }
      }
    }
    if (in_class && al_has(&cls_func_iter)) {
      FunctionRef *func_ref = (FunctionRef *)al_value(&cls_func_iter);
      if (func_ref->index == i) {
        fprintf(file, "@%s\n", func_ref->name);
        al_inc(&cls_func_iter);
      }
    } else if (al_has(&func_iter)) {
      FunctionRef *func_ref = (FunctionRef *)al_value(&func_iter);
      if (func_ref->index == i) {
        fprintf(file, "@%s\n", func_ref->name);
        al_inc(&func_iter);
      }
    }
    if (i < alist_len(&tape->ins)) {
      Instruction *ins = alist_get(&tape->ins, i);
      int chars_written = instruction_write(ins, file);
      SourceMapping *sm = (SourceMapping *)alist_get(&tape->source_map, i);
      if (sm->col >= 0 && sm->line >= 0) {
        int lpadding = max(INSTRUCTION_COMMENT_LPAD - chars_written, 0);
        fprintf(file, "%*s #%d %d", lpadding, PADDING, sm->line, sm->col);
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
    if (NULL != head->current_class) {
      keyedlist_insert(&head->current_class->func_refs, old_func->name,
                       (void **)&cpy);
    } else {
      keyedlist_insert(&head->func_refs, old_func->name, (void **)&cpy);
    }
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
    // Copy all fields.
    KL_iter field_iter = keyedlist_iter(&old_class->field_refs);
    for (; kl_has(&field_iter); kl_inc(&field_iter)) {
      FieldRef *old_field = (FieldRef *)kl_value(&field_iter);
      FieldRef *cpy;
      keyedlist_insert(&cpy_class->field_refs, old_field->name, (void **)&cpy);
      *cpy = *old_field;
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
  while (first->type == TOKEN_NEWLINE) {
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
  if (SYMBOL_AT == first->type) {
    Token *fn_name = Q_remove(tokens, 0);
    if (SYMBOL_COLON == _q_peek(tokens)->type) {
      Q_remove(tokens, 0);
      Token *async_keyword = Q_remove(tokens, 0);
      if (0 != strcmp("async", async_keyword->text)) {
        FATALF("Invalid function qualifier '%s' on '%s'.", async_keyword->text,
               fn_name->text);
      }
      tape_label_async(tape, fn_name);
    }
    if (TOKEN_NEWLINE != _q_peek(tokens)->type) {
      FATALF("Invalid token after @def.");
    }
    tape_label(tape, fn_name);
    return;
  }
  if (0 == strcmp(CLASS_KEYWORD, first->text)) {
    Token *class_name = Q_remove(tokens, 0);
    if (TOKEN_NEWLINE == _q_peek(tokens)->type) {
      tape_class(tape, class_name);
      return;
    }
    Q parents;
    Q_init(&parents);
    while (SYMBOL_COMMA == _q_peek(tokens)->type) {
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
  if (0 == strcmp(FIELD_KEYWORD, first->text)) {
    Token *field = Q_remove(tokens, 0);
    tape_field(tape, field->text);
    return;
  }
  Op op = str_to_op(first->text);
  Token *next = (Token *)_q_peek(tokens);
  if (TOKEN_NEWLINE == next->type || SYMBOL_POUND == next->type) {
    tape_ins_no_arg(tape, op, first);
  } else if (SYMBOL_MINUS == next->type) {
    Q_remove(tokens, 0);
    tape_ins_neg(tape, op, Q_remove(tokens, 0));
  } else {
    Q_remove(tokens, 0);
    tape_ins(tape, op, next);
  }

  next = (Token *)_q_peek(tokens);
  if (next != NULL && SYMBOL_POUND == next->type) {
    Q_remove(tokens, 0);
    Primitive line = token_to_primitive(Q_remove(tokens, 0));
    Primitive col = token_to_primitive(Q_remove(tokens, 0));
    SourceMapping *sm =
        tape_add_source(tape, tape_get_mutable(tape, tape_size(tape) - 1));
    sm->source_line = pint(&line);
    sm->source_col = pint(&col);
    sm->source_token =
        token_create(sm->token->type, sm->source_line, sm->source_col,
                     sm->token->text, strlen(sm->token->text));
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
  if (TOKEN_NEWLINE == ((Token *)_q_peek(tokens))->type) {
    Q_remove(tokens, 0);
  }
  if (0 == strcmp(SOURCE_KEYWORD, ((Token *)_q_peek(tokens))->text)) {
    Q_remove(tokens, 0);
    Token *tok = (Token *)Q_remove(tokens, 0);
    ASSERT(NOT_NULL(tok));
    tape_set_external_source(tape, tok->text);
  }
  while (Q_size(tokens) > 0) {
    tape_read_ins(tape, tokens);
  }
}

// **********************
// Specialized functions.
// **********************

int tape_ins_raw(Tape *tape, Instruction *ins) {
  ASSERT(NOT_NULL(tape));
  Instruction *new_ins = tape_add(tape);
  *new_ins = *ins;
  tape_add_source(tape, new_ins);
  return 1;
}

Primitive token_to_primitive(const Token *tok) {
  ASSERT_NOT_NULL(tok);
  Primitive val;
  switch (tok->type) {
  case TOKEN_INTEGER:
    val._type = PRIMITIVE_INT;
    val._int_val = (int64_t)strtoll(tok->text, NULL, 10);
    break;
  case TOKEN_FLOATING:
    val._type = PRIMITIVE_FLOAT;
    val._float_val = strtod(tok->text, NULL);
    break;
  default:
    FATALF("Attempted to create a Value from '%s'.", tok->text);
  }
  return val;
}

int tape_ins(Tape *tape, Op op, const Token *token) {
  ASSERT(NOT_NULL(tape), NOT_NULL(token));
  char *unescaped_str;
  Instruction *ins = tape_add(tape);
  SourceMapping *sm = tape_add_source(tape, ins);

  ins->op = op;
  sm->token = token;
  sm->line = token->line;
  sm->col = token->col;

  switch (token->type) {
  case TOKEN_INTEGER:
  case TOKEN_FLOATING:
    ins->type = INSTRUCTION_PRIMITIVE;
    ins->val = token_to_primitive(token);
    break;
  case TOKEN_STRING:
    ins->type = INSTRUCTION_STRING;
    unescaped_str = unescape(token->text);
    ins->str = intern(unescaped_str);
    DEALLOC(unescaped_str);
    break;
  case TOKEN_WORD:
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
  ins->val._type = PRIMITIVE_INT;
  ins->val._int_val = val;
  return 1;
}

DEB_FN(int, tape_ins_no_arg, Tape *tape, Op op, const Token *token) {
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
  _tape_start_func(tape, token->text, /*is_async=*/false);
  return 0;
}

int tape_label_async(Tape *tape, const Token *token) {
  _tape_start_func(tape, token->text, /*is_async=*/true);
  return 0;
}

int tape_label_text(Tape *tape, const char text[]) {
  _tape_start_func(tape, text, /*is_async=*/false);
  return 0;
}

int tape_label_text_async(Tape *tape, const char text[]) {
  _tape_start_func(tape, text, /*is_async=*/true);
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

int tape_anon_label_async(Tape *tape, const Token *token) {
  char *label = anon_fn_for_token(token);
  tape_label_text_async(tape, label);
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