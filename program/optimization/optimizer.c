// optimizer.c
//
// Created on: Jan 13, 2018
//     Author: Jeff

#include "program/optimization/optimizer.h"

#include "debug/debug.h"

void add_adjustment(OptimizeHelper *oh, Adjustment *a, int index) {
  int i = alist_append(oh->adjustments, a);
  map_insert(&oh->i_to_adj, as_ptr(index), as_ptr(i));
}

void add_insertion(OptimizeHelper *oh, Adjustment *a) {
  int i = alist_append(oh->adjustments, a);
  map_insert(&oh->inserts, as_ptr(a->insert_pos), as_ptr(i));
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
