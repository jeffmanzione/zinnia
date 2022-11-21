// optimize.c
//
// Created on: Jan 13, 2018
//     Author: Jeff

#include "program/optimization/optimize.h"

#include <stddef.h>
#include <stdint.h>

#include "debug/debug.h"
#include "entity/entity.h"
#include "lang/lexer/lang_lexer.h"
#include "lang/lexer/token.h"
#include "program/instruction.h"
#include "program/optimization/optimizers.h"
#include "struct/alist.h"
#include "struct/map.h"
#include "struct/q.h"
#include "struct/set.h"
#include "struct/struct_defaults.h"

#define is_goto(op)                                                            \
  (((op) == JMP) || ((op) == IFN) || ((op) == IF) || ((op) == CTCH))

static AList *optimizers = NULL;

void optimize_init() {
  optimizers = alist_create(Optimizer, 64);
  register_optimizer("ResPush", optimizer_ResPush);
  register_optimizer("SetRes", optimizer_SetRes);
  register_optimizer("SetPush", optimizer_SetPush);
  register_optimizer("JmpRes", optimizer_JmpRes);
  register_optimizer("PushRes", optimizer_PushRes);
  register_optimizer("ResPush2", optimizer_ResPush2);
  register_optimizer("RetRet", optimizer_RetRet);
  register_optimizer("PeekRes", optimizer_PeekRes);
  register_optimizer("PeekPush", optimizer_PeekPush);
  register_optimizer("SetEmpty", optimizer_SetEmpty);
  register_optimizer("PeekPeek", optimizer_PeekPeek);
  register_optimizer("PushResEmpty", optimizer_PushResEmpty);
  register_optimizer("PeekRes-SecondPass", optimizer_PeekRes);
  register_optimizer("PushRes2", optimizer_PushRes2);
  register_optimizer("SimpleMath", optimizer_SimpleMath);
  register_optimizer("GetPush", optimizer_GetPush);
  register_optimizer("Nil", optimizer_Nil);
  register_optimizer("ResAidx", optimizer_ResAidx);
  register_optimizer("Increment", optimizer_Increment);
  register_optimizer("StringConcat", optimizer_StringConcat);
}

void optimize_finalize() { alist_delete(optimizers); }

void _populate_gotos(OptimizeHelper *oh) {
  map_init_default(&oh->i_gotos);
  int i, len = tape_size(oh->tape);
  for (i = 0; i < len; i++) {
    const Instruction *ins = tape_get(oh->tape, i);
    if (!is_goto(ins->op)) {
      continue;
    }
    int index = i + pint(&ins->val);
    map_insert(&oh->i_gotos, as_ptr(index), as_ptr(i));
  }
}

void _tape_populate_mappings(const Tape *tape, Map *i_to_refs,
                             Map *i_to_class_starts, Map *i_to_class_ends) {
  map_init_default(i_to_refs);
  map_init_default(i_to_class_starts);
  map_init_default(i_to_class_ends);

  KL_iter refs_iter = tape_functions(tape);
  for (; kl_has(&refs_iter); kl_inc(&refs_iter)) {
    const FunctionRef *fref = (const FunctionRef *)kl_value(&refs_iter);
    map_insert(i_to_refs, as_ptr(fref->index), fref);
  }

  KL_iter class_iter = tape_classes(tape);
  for (; kl_has(&class_iter); kl_inc(&class_iter)) {
    const ClassRef *cref = (const ClassRef *)kl_value(&class_iter);
    map_insert(i_to_class_starts, as_ptr(cref->start_index), cref->name);
    map_insert(i_to_class_ends, as_ptr(cref->end_index), cref->name);
    KL_iter methods_iter = keyedlist_iter((KeyedList *)&cref->func_refs);
    for (; kl_has(&methods_iter); kl_inc(&methods_iter)) {
      const FunctionRef *fref = (const FunctionRef *)kl_value(&methods_iter);
      map_insert(i_to_refs, as_ptr(fref->index), fref);
    }
  }
}

void _tape_clear_mappings(Map *i_to_refs, Map *i_to_class_starts,
                          Map *i_to_class_ends) {
  map_finalize(i_to_refs);
  map_finalize(i_to_class_starts);
  map_finalize(i_to_class_ends);
}

void oh_init(OptimizeHelper *oh, const Tape *tape) {
  oh->tape = tape;
  oh->adjustments = alist_create(Adjustment, 128);
  map_init_default(&oh->i_to_adj);
  map_init_default(&oh->inserts);
  _populate_gotos(oh);
  _tape_populate_mappings(oh->tape, &oh->i_to_refs, &oh->i_to_class_starts,
                          &oh->i_to_class_ends);
}

void _oh_resolve(OptimizeHelper *oh, Tape *new_tape) {
  const Tape *t = oh->tape;
  Token tok;
  token_fill(&tok, TOKEN_WORD, 0, 0, tape_module_name(t),
             strlen(tape_module_name(t)));
  tape_module(new_tape, &tok);
  tape_set_external_source(new_tape, tape_get_external_source(t));
  int i, old_len = tape_size(t);
  AList *old_index = alist_create(int, DEFAULT_ARRAY_SZ);
  AList *new_index = alist_create(int, DEFAULT_ARRAY_SZ);
  for (i = 0; i <= old_len; ++i) {
    char *text = NULL;
    if (NULL != (text = map_lookup(&oh->i_to_class_ends, as_ptr(i)))) {
      Token tok;
      token_fill(&tok, TOKEN_WORD, 0, 0, text, strlen(text));
      tape_endclass(new_tape, &tok);
    }
    if (NULL != (text = map_lookup(&oh->i_to_class_starts, as_ptr(i)))) {
      Token tok;
      token_fill(&tok, TOKEN_WORD, 0, 0, text, strlen(text));
      const ClassRef *cref = tape_get_class(t, text);
      if (0 == alist_len(&cref->supers)) {
        tape_class(new_tape, &tok);
      } else {
        Q q_parents;
        Q_init(&q_parents);
        AL_iter parent_classes = alist_iter(&cref->supers);
        for (; al_has(&parent_classes); al_inc(&parent_classes)) {
          Q_enqueue(&q_parents, *((char **)al_value(&parent_classes)));
        }
        tape_class_with_parents(new_tape, &tok, &q_parents);
        Q_finalize(&q_parents);
      }
      KL_iter fields = keyedlist_iter((KeyedList *)&cref->field_refs);
      for (; kl_has(&fields); kl_inc(&fields)) {
        FieldRef *fref = (FieldRef *)kl_value(&fields);
        tape_field(new_tape, fref->name);
      }
    }
    FunctionRef *fref;
    if (NULL != (fref = map_lookup(&oh->i_to_refs, as_ptr(i)))) {
      if (fref->is_async) {
        tape_label_text_async(new_tape, fref->name);
      } else {
        tape_label_text(new_tape, fref->name);
      }
    }
    // Prevents out of bounds.
    if (i >= old_len) {
      break;
    }
    const Instruction *ins = tape_get(t, i);
    // instruction_write(ins, stdout);
    // printf(" <-- o\n");
    int new_len = tape_size(new_tape);
    void *insert_index = map_lookup(&oh->inserts, as_ptr(i + 1));
    if (NULL != insert_index) {
      Adjustment *insert =
          alist_get(oh->adjustments, (int)(intptr_t)insert_index - 1);
      int j;
      for (j = insert->start; j < insert->end; j++) {
        alist_append(new_index, &new_len);
        alist_append(old_index, &j);
        Instruction *new_ins = tape_add(new_tape);
        *new_ins = *tape_get(t, j);
        *tape_add_source(new_tape, new_ins) = *tape_get_source(t, j);
      }
    }
    void *a_index = map_lookup(&oh->i_to_adj, as_ptr(i + 1));
    Adjustment *a = NULL;
    if (NULL != a_index) {
      a = alist_get(oh->adjustments, (int)(intptr_t)a_index - 1);
    }
    if (NULL != a && REMOVE == a->type) {
      int new_index_val = new_len - 1;
      alist_append(new_index, &new_index_val);
      continue;
    }
    alist_append(new_index, &new_len);
    alist_append(old_index, &i);
    Instruction *new_ins = tape_add(new_tape);
    *tape_add_source(new_tape, new_ins) = *tape_get_source(t, i);
    *new_ins = *ins;
    if (NULL != a) {
      if (SET_OP == a->type) {
        new_ins->op = a->op;
      } else if (SET_VAL == a->type) {
        new_ins->op = a->op;
        new_ins->type = INSTRUCTION_PRIMITIVE;
        new_ins->val = a->val;
      } else if (REPLACE == a->type) {
        *new_ins = a->ins;
      }
    }
  }
  int new_len = tape_size(new_tape);
  for (i = 0; i < new_len; i++) {
    Instruction *ins = tape_get_mutable(new_tape, i);
    if (!is_goto(ins->op)) {
      continue;
    }
    ASSERT(INSTRUCTION_PRIMITIVE == ins->type);
    int diff = pint(&ins->val);
    int old_i = *((int *)alist_get(old_index, i));
    int old_goto_i = old_i + diff;
    int new_goto_i = *((int *)alist_get(new_index, old_goto_i));
    ins->val = primitive_int(new_goto_i - i);
  }
  alist_delete(old_index);
  alist_delete(new_index);
  _tape_clear_mappings(&oh->i_to_refs, &oh->i_to_class_starts,
                       &oh->i_to_class_ends);
  map_finalize(&oh->i_to_adj);
  map_finalize(&oh->inserts);
  map_finalize(&oh->i_gotos);
  alist_delete(oh->adjustments);
}

Tape *optimize(Tape *const t) {
  Tape *tape = t;
  int i, opts_len = alist_len(optimizers);
  for (i = 0; i < opts_len; ++i) {
    OptimizeHelper oh;
    oh_init(&oh, tape);
    (*((Optimizer *)alist_get(optimizers, i)))(&oh, tape, 0, tape_size(tape));
    Tape *new_tape = tape_create();
    _oh_resolve(&oh, new_tape);
    tape_delete(tape);
    tape = new_tape;
  }
  return tape;
}

void register_optimizer(const char name[], const Optimizer o) {
  alist_append(optimizers, &o);
}

void Int32_swap(void *x, void *y) {
  int32_t *cx = x;
  int32_t *cy = y;
  int32_t tmp = *cx;
  *cx = *cy;
  *cy = tmp;
}

int Int32_compare(void *x, void *y) {
  return *((int32_t *)x) - *((int32_t *)y);
}
