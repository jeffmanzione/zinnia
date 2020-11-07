// optimizer.h
//
// Created on: Jan 13, 2018
//     Author: Jeff

#ifndef PROGRAM_OPTIMIZATION_OPTIMIZER_H_
#define PROGRAM_OPTIMIZATION_OPTIMIZER_H_

#include <stdint.h>

#include "program/instruction.h"
#include "program/tape.h"
#include "struct/alist.h"
#include "struct/map.h"

typedef struct {
  enum { SET_OP, REMOVE, SHIFT, SET_VAL, REPLACE } type;
  union {
    Op op;
    Instruction ins;
  };
  struct {
    uint32_t start, end, insert_pos;
  };
  Primitive val;
} Adjustment;

typedef struct {
  const Tape *tape;
  Map i_to_adj, i_gotos, inserts;
  Map i_to_refs, i_to_class_starts, i_to_class_ends;
  AList *adjustments;
} OptimizeHelper;

typedef void (*Optimizer)(OptimizeHelper *, const Tape *, int, int);

void o_Remove(OptimizeHelper *oh, int index);
void o_Replace(OptimizeHelper *oh, int index, Instruction ins);
void o_SetOp(OptimizeHelper *oh, int index, Op op);
void o_SetVal(OptimizeHelper *oh, int index, Op op, Primitive val);
void o_Shift(OptimizeHelper *oh, int start_index, int end_index, int new_index);

#endif /* PROGRAM_OPTIMIZATION_OPTIMIZER_H_ */
