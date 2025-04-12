// instruction.h
//
// Created on: Jun 3, 2020
//     Author: Jeff Manzione

#ifndef PROGRAM_INSTRUCTION_H_
#define PROGRAM_INSTRUCTION_H_

#include <stdio.h>

#include "entity/primitive.h"
#include "program/op.h"

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

int instruction_write(const Instruction *ins, FILE *file, bool minimize);

#endif /* PROGRAM_INSTRUCTION_H_ */