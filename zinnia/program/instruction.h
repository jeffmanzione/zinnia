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

// How the VM runs an instruction together with the one after it, as one. Set
// by bytecode_fuse() when the VM loads code; files never store it.
typedef enum {
  FUSE_NONE = 0,
  // A comparison, then the IF or IFN that branches on it.
  FUSE_BRANCH,
  // An instruction that goes on to the next, then a JMP.
  FUSE_JMP,
  // A function's SLOTS, then the LET of its only parameter.
  FUSE_LET,
  // An ARGC, then the instructions that move its arguments into their slots
  // (IFN, then RES and LET for each, then JMP), when they're on the stack.
  // Those all run as one.
  FUSE_ARGS,
} Fuse;

typedef struct {
  char op;
  char type;
  // A Fuse.
  uint8_t fuse;
  // For an INSTRUCTION_ID operand that names a variable kept in a slot, the
  // slot's number + 1, and 0 otherwise; the id still holds the name. For SLOTS,
  // the number of slots.
  uint16_t slot;
  union {
    Primitive val;
    const char *id;
    const char *str;
  };
  // What the VM found for the instruction last time, so it can skip finding it
  // again: for a call of a method by name, the Function found on its object's
  // own class; for a name looked up in its module's members, that member's
  // Value. The VM sets it, on whichever threads run the instruction, so it's
  // only read and written atomically. Always NULL in a compiled file.
  const void *cache;
} Instruction;

DEFINE_ARRAYLIKE(InstructionArray, Instruction);

int instruction_write(const Instruction *ins, FILE *file, bool minimize);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_INSTRUCTION_H_ */