// instruction.h
//
// Created on: Jun 3, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_INSTRUCTION_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_INSTRUCTION_H_

#include <stdio.h>

#include "c-data-structures/arraylike.h"
#include "zinnia/entity/primitive.h"
#include "zinnia/program/op.h"

typedef enum {
  INSTRUCTION_NO_ARG,
  INSTRUCTION_ID,
  INSTRUCTION_STRING,
  INSTRUCTION_PRIMITIVE
} InstructionType;

typedef struct {
  char op;
  char type;
  union {
    Primitive val;
    const char *id;
    const char *str;
  };
} Instruction;

DEFINE_ARRAYLIKE(InstructionArray, Instruction);

int instruction_write(const Instruction *ins, FILE *file, bool minimize);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_INSTRUCTION_H_ */