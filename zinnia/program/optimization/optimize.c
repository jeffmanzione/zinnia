// optimize.c
//
// Created on: Jan 13, 2018
//     Author: Jeff

#include "zinnia/program/optimization/optimize.h"

#include <stddef.h>
#include <stdint.h>

#include "c-data-structures/arraylike.h"
#include "language-tools/lexer/token.h"
#include "zinnia/entity/entity.h"
#include "zinnia/lang/lexer/lang_lexer.h"
#include "zinnia/program/instruction.h"
#include "zinnia/program/optimization/optimizers.h"
#include "zinnia/util/error.h"
#include "zinnia/util/void_array.h"

DEFINE_ARRAYLIKE(OptimizerArray, Optimizer);
IMPL_ARRAYLIKE(OptimizerArray, Optimizer);

#define is_goto(op) \
  (((op) == JMP) || ((op) == IFN) || ((op) == IF) || ((op) == CTCH))

static OptimizerArray optimizers;

void optimize_init() {
  OptimizerArray_init_capacity(&optimizers, 64);
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

void optimize_finalize() { OptimizerArray_finalize(&optimizers); }

void populate_gotos_(OptimizeHelper *oh) {
  IntIntMap_init(&oh->i_gotos, hash_int, compare_ints);
  int i, len = tape_size(oh->tape);
  for (i = 0; i < len; i++) {
    const Instruction *ins = tape_get(oh->tape, i);
    if (!is_goto(ins->op)) {
      continue;
    }
    int index = i + pint(&ins->val);
    IntIntMap_insert(&oh->i_gotos, index, sizeof(int), i);
  }
}

void tape_populate_mappings_(const Tape *tape, IntToFunctionRefMap *i_to_refs,
                             IntToCharPtrArrayMap *i_to_class_starts,
                             IntToCharPtrArrayMap *i_to_class_ends) {
  IntToFunctionRefMap_init(i_to_refs, hash_int, compare_ints);
  IntToCharPtrArrayMap_init(i_to_class_starts, hash_int, compare_ints);
  IntToCharPtrArrayMap_init(i_to_class_ends, hash_int, compare_ints);

  FunctionRefMapIOIterator refs_iter = tape_functions(tape);
  for (; FunctionRefMap_io_has_next(&refs_iter);
       FunctionRefMap_io_next(&refs_iter)) {
    const FunctionRef *fref = FunctionRefMap_io_value(&refs_iter);
    IntToFunctionRefMap_insert(i_to_refs, fref->index, sizeof(int), fref);
  }

  ClassRefMapIOIterator class_iter = tape_classes(tape);
  for (; ClassRefMap_io_has_next(&class_iter);
       ClassRefMap_io_next(&class_iter)) {
    const ClassRef *cref = ClassRefMap_io_value(&class_iter);

    {
      CharPtrArray *starts;
      if (IntToCharPtrArrayMap_insert(i_to_class_starts, cref->start_index,
                                      sizeof(int), &starts)) {
        CharPtrArray_init(starts);
      }
      *CharPtrArray_push_back_ref(starts) = (char *)cref->name;
    }

    {
      CharPtrArray *ends;
      if (IntToCharPtrArrayMap_insert(i_to_class_ends, cref->end_index,
                                      sizeof(int), &ends)) {
        CharPtrArray_init(ends);
      }
      *CharPtrArray_push_back_ref(ends) = (char *)cref->name;
    }

    FunctionRefMapIOIterator methods_iter;
    FunctionRefMap_io_iterator(&methods_iter, &cref->func_refs);
    for (; FunctionRefMap_io_has_next(&methods_iter);
         FunctionRefMap_io_next(&methods_iter)) {
      const FunctionRef *fref = FunctionRefMap_io_value(&methods_iter);
      IntToFunctionRefMap_insert(i_to_refs, fref->index, sizeof(int), fref);
    }
  }
}

void tape_clear_mappings_(IntToFunctionRefMap *i_to_refs,
                          IntToCharPtrArrayMap *i_to_class_starts,
                          IntToCharPtrArrayMap *i_to_class_ends) {
  IntToFunctionRefMap_finalize(i_to_refs);

  IntToCharPtrArrayMapIterator starts;
  IntToCharPtrArrayMap_iterator(&starts, i_to_class_starts);
  for (; IntToCharPtrArrayMap_has_entry(&starts);
       IntToCharPtrArrayMap_next_entry(&starts)) {
    CharPtrArray_finalize(IntToCharPtrArrayMap_mutable_value(&starts));
  }
  IntToCharPtrArrayMap_finalize(i_to_class_starts);

  IntToCharPtrArrayMapIterator ends;
  IntToCharPtrArrayMap_iterator(&ends, i_to_class_ends);
  for (; IntToCharPtrArrayMap_has_entry(&ends);
       IntToCharPtrArrayMap_next_entry(&ends)) {
    CharPtrArray_finalize(IntToCharPtrArrayMap_mutable_value(&ends));
  }
  IntToCharPtrArrayMap_finalize(i_to_class_ends);
}

void oh_init(OptimizeHelper *oh, const Tape *tape) {
  oh->tape = tape;
  AdjustmentArray_init(&oh->adjustments);
  IntIntMap_init(&oh->i_to_adj, hash_int, compare_ints);
  IntIntMap_init(&oh->inserts, hash_int, compare_ints);
  populate_gotos_(oh);
  tape_populate_mappings_(oh->tape, &oh->i_to_refs, &oh->i_to_class_starts,
                          &oh->i_to_class_ends);
}

void oh_resolve_(OptimizeHelper *oh, Tape *new_tape) {
  const Tape *t = oh->tape;
  Token tok;
  token_fill(&tok, TOKEN_WORD, 0, 0, tape_module_name(t),
             strlen(tape_module_name(t)));
  tape_module(new_tape, &tok);
  tape_set_external_source(new_tape, tape_get_external_source(t));
  int i, old_len = tape_size(t);
  IntArray old_index;
  IntArray_init(&old_index);
  IntArray new_index;
  IntArray_init(&new_index);
  bool in_class = false;
  for (i = 0; i <= old_len; ++i) {
    char *class_name;
    CharPtrArray *class_ends = NULL;
    if (in_class && NULL != (class_ends = IntToCharPtrArrayMap_find_ref(
                                 &oh->i_to_class_ends, i, sizeof(int)))) {
      in_class = false;
      CharPtrArrayIterator it;
      CharPtrArray_iterator(&it, class_ends);
      for (; CharPtrArray_has_next(&it); CharPtrArray_next(&it)) {
        class_name = *CharPtrArray_mutable_value(&it);
        Token tok;
        token_fill(&tok, TOKEN_WORD, 0, 0, class_name, strlen(class_name));
        tape_endclass(new_tape, &tok);
        // Only process first one.
        break;
      }
    }
    CharPtrArray *starts =
        IntToCharPtrArrayMap_find_ref(&oh->i_to_class_starts, i, sizeof(int));
    if (!in_class && NULL != starts) {
      CharPtrArrayIterator iter;
      CharPtrArray_iterator(&iter, starts);
      for (; CharPtrArray_has_next(&iter); CharPtrArray_next(&iter)) {
        in_class = true;
        class_name = *CharPtrArray_mutable_value(&iter);
        Token tok;
        token_fill(&tok, TOKEN_WORD, 0, 0, class_name, strlen(class_name));
        const ClassRef *cref = tape_get_class(t, class_name);
        if (0 == CharPtrArray_size(&cref->supers)) {
          tape_class(new_tape, &tok);
        } else {
          CharPtrArray q_parents;
          CharPtrArray_init(&q_parents);
          CharPtrArrayIterator parent_classes;
          CharPtrArray_iterator(&parent_classes, &cref->supers);
          for (; CharPtrArray_has_next(&parent_classes);
               CharPtrArray_next(&parent_classes)) {
            CharPtrArray_push_back(
                &q_parents, *CharPtrArray_mutable_value(&parent_classes));
          }
          tape_class_with_parents(new_tape, &tok, &q_parents);
          CharPtrArray_finalize(&q_parents);
        }
        FieldRefMapIOIterator fields;
        FieldRefMap_io_iterator(&fields, &cref->field_refs);
        for (; FieldRefMap_io_has_next(&fields); FieldRefMap_io_next(&fields)) {
          const FieldRef *fref = FieldRefMap_io_value(&fields);
          tape_field(new_tape, fref->name);
        }
        // Handles case where class has no body.
        CharPtrArray *ends =
            IntToCharPtrArrayMap_find_ref(&oh->i_to_class_ends, i, sizeof(int));
        if (NULL != ends) {
          CharPtrArrayIterator end_iter;
          CharPtrArray_iterator(&end_iter, ends);
          for (; CharPtrArray_has_next(&end_iter);
               CharPtrArray_next(&end_iter)) {
            if (0 == strcmp(class_name, *CharPtrArray_value(&end_iter))) {
              in_class = false;
              tape_endclass(new_tape, &tok);
              break;
            }
          }
        }
      }
    }
    FunctionRef *fref;
    if (NULL != (fref = IntToFunctionRefMap_find(&oh->i_to_refs, i, sizeof(int),
                                                 NULL))) {
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
    const int insert_index =
        IntIntMap_find(&oh->inserts, i + 1, sizeof(int), -1);
    if (insert_index >= 0) {
      Adjustment *insert = AdjustmentArray_mutable_ref_unchecked(
          &oh->adjustments, insert_index - 1);
      int j;
      for (j = insert->start; j < insert->end; j++) {
        IntArray_push_back(&new_index, new_len);
        IntArray_push_back(&old_index, j);
        Instruction *new_ins = tape_add(new_tape);
        *new_ins = *tape_get(t, j);
        *tape_add_source(new_tape, new_ins) = *tape_get_source(t, j);
      }
    }
    const int a_index = IntIntMap_find(&oh->i_to_adj, i + 1, sizeof(int), -1);
    Adjustment *a = NULL;
    if (a_index >= 0) {
      a = AdjustmentArray_mutable_ref_unchecked(&oh->adjustments, a_index - 1);
    }
    if (NULL != a && REMOVE == a->type) {
      int new_index_val = new_len - 1;
      IntArray_push_back(&new_index, new_index_val);
      continue;
    }
    IntArray_push_back(&new_index, new_len);
    IntArray_push_back(&old_index, i);
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
    int old_i = IntArray_get_unchecked(&old_index, i);
    int old_goto_i = old_i + diff;
    int new_goto_i = IntArray_get_unchecked(&new_index, old_goto_i);
    ins->val = primitive_int(new_goto_i - i);
  }
  IntArray_finalize(&old_index);
  IntArray_finalize(&new_index);
  tape_clear_mappings_(&oh->i_to_refs, &oh->i_to_class_starts,
                       &oh->i_to_class_ends);
  IntIntMap_finalize(&oh->i_to_adj);
  IntIntMap_finalize(&oh->inserts);
  IntIntMap_finalize(&oh->i_gotos);
  AdjustmentArray_finalize(&oh->adjustments);
}

Tape *optimize(Tape *const t) {
  Tape *tape = t;
  int i, opts_len = OptimizerArray_size(&optimizers);
  for (i = 0; i < opts_len; ++i) {
    OptimizeHelper oh;
    oh_init(&oh, tape);
    OptimizerArray_get_unchecked(&optimizers, i)(&oh, tape, 0, tape_size(tape));
    Tape *new_tape = tape_create();
    oh_resolve_(&oh, new_tape);
    tape_delete(tape);
    tape = new_tape;
  }
  return tape;
}

void register_optimizer(const char name[], const Optimizer o) {
  OptimizerArray_push_back(&optimizers, o);
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
