// optimizer.h
//
// Created on: Jan 13, 2018
//     Author: Jeff

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_OPTIMIZATION_OPTIMIZER_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_OPTIMIZATION_OPTIMIZER_H_

#include <stdint.h>

#include "c-data-structures/arraylike.h"
#include "c-data-structures/maplike.h"
#include "zinnia/program/instruction.h"
#include "zinnia/program/tape.h"

DEFINE_MAPLIKE(IntIntMap, int, int);
DEFINE_MAPLIKE(IntCharPtrMap, int, char *);
DEFINE_MAPLIKE(IntToFunctionRefMap, int, FunctionRef *);
DEFINE_STABLE_MAPLIKE(IntToCharPtrArrayMap, int, CharPtrArray);

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

DEFINE_ARRAYLIKE(AdjustmentArray, Adjustment);

typedef struct {
  const Tape *tape;
  IntIntMap i_to_adj, i_gotos, inserts;
  IntToFunctionRefMap i_to_refs;
  IntToCharPtrArrayMap i_to_class_starts, i_to_class_ends;
  AdjustmentArray adjustments;
} OptimizeHelper;

typedef void (*Optimizer)(OptimizeHelper *, const Tape *, int, int);

void o_Remove(OptimizeHelper *oh, int index);
void o_Replace(OptimizeHelper *oh, int index, Instruction ins);
void o_SetOp(OptimizeHelper *oh, int index, Op op);
void o_SetVal(OptimizeHelper *oh, int index, Op op, Primitive val);
void o_Shift(OptimizeHelper *oh, int start_index, int end_index, int new_index);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_OPTIMIZATION_OPTIMIZER_H_ */
