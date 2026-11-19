// instruction.h
//
// Created on: Jun 3, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_INSTRUCTION_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_INSTRUCTION_H_

#include <stdio.h>

#include <stdint.h>

#include "c-data-structures/arraylike.h"
#include "zinnia/program/op.h"
#include "zinnia/value/primitive.h"

typedef enum {
  INSTRUCTION_NO_ARG,
  INSTRUCTION_ID,
  INSTRUCTION_STRING,
  INSTRUCTION_PRIMITIVE
} InstructionType;

typedef struct {
  char op;
  char type;
  // For an INSTRUCTION_ID operand that names a variable kept in a slot, the
  // slot's number + 1, and 0 otherwise; the id still holds the name. For SLOTS,
  // the number of slots.
  uint16_t slot;
  union {
    Primitive val;
    const char *id;
    const char *str;
  };
  // For a call of a method by name, the Function the last call found on its
  // object's own class, so the next call on that class skips the lookup. The
  // VM sets it, on whichever threads run the instruction, so it's only read and
  // written atomically. Always NULL in a compiled file.
  const void *method_cache;
} Instruction;

DEFINE_ARRAYLIKE(InstructionArray, Instruction);

int instruction_write(const Instruction *ins, FILE *file, bool minimize);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_INSTRUCTION_H_ */