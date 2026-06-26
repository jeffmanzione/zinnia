// optimizer.c
//
// Created on: Jan 13, 2018
//     Author: Jeff

#include "zinnia/program/optimization/optimizer.h"

#include "zinnia/util/error.h"

IMPL_ARRAYLIKE(AdjustmentArray, Adjustment);
IMPL_MAPLIKE(IntIntMap, int, int);
IMPL_MAPLIKE(IntCharPtrMap, int, char *);
IMPL_MAPLIKE(IntToFunctionRefMap, int, FunctionRef *);
IMPL_STABLE_MAPLIKE(IntToCharPtrArrayMap, int, CharPtrArray);

void add_adjustment(OptimizeHelper *oh, Adjustment *a, int index) {
  *AdjustmentArray_push_back_ref(&oh->adjustments) = *a;
  IntIntMap_insert(&oh->i_to_adj, index + 1, sizeof(int),
                   AdjustmentArray_size(&oh->adjustments));
}

void add_insertion(OptimizeHelper *oh, Adjustment *a) {
  *AdjustmentArray_push_back_ref(&oh->adjustments) = *a;
  IntIntMap_insert(&oh->inserts, a->insert_pos + 1, sizeof(int),
                   AdjustmentArray_size(&oh->adjustments));
}

void o_Remove(OptimizeHelper *oh, int index) {
  Adjustment a = {.type = REMOVE, .op = NOP};
  add_adjustment(oh, &a, index);
}

void o_Replace(OptimizeHelper *oh, int index, Instruction ins) {
  Adjustment a = {.type = REPLACE, .ins = ins};
  add_adjustment(oh, &a, index);
}

void o_SetOp(OptimizeHelper *oh, int index, Op op) {
  Adjustment a = {.type = SET_OP, .op = op};
  add_adjustment(oh, &a, index);
}

void o_SetVal(OptimizeHelper *oh, int index, Op op, Primitive val) {
  Adjustment a = {.type = SET_VAL, .op = op, .val = val};
  add_adjustment(oh, &a, index);
}

void o_Shift(OptimizeHelper *oh, int start_index, int end_index,
             int insert_pos) {
  Adjustment a = {.type = SET_OP,
                  .start = start_index,
                  .end = end_index,
                  .insert_pos = insert_pos};
  add_insertion(oh, &a);
  int i;
  for (i = start_index; i < end_index; i++) {
    o_Remove(oh, i);
  }
}
