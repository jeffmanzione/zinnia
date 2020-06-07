// instruction.h
//
// Created on: Jun 6, 2020
//     Author: Jeff Manzione

#ifndef PROGRAM_TAPE_H_
#define PROGRAM_TAPE_H_

#include "program/instruction.h"

typedef struct {
  // TODO: Something that points to the file, func, etc...
  int32_t line, col;
} SourceMapping;

typedef struct _Tape Tape;

Tape *tape_create();
void tape_delete(Tape *tape);
Instruction *tape_add(Tape *tape);
SourceMapping *tape_add_source(Tape *tape, Instruction *ins);

void tape_start_func(Tape *tape, const char name[]);
void tape_start_class(Tape *tape, const char name[]);
void tape_end_class(Tape *tape);

const Instruction *tape_get(Tape *tape, uint32_t index);
const SourceMapping *tape_get_source(Tape *tape, uint32_t index);
size_t tape_size(const Tape *tape);

void tape_write(const Tape *tape, FILE *file);
#endif /* PROGRAM_TAPE_H_ */