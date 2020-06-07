// instruction.h
//
// Created on: Jun 6, 2020
//     Author: Jeff Manzione

#ifndef PROGRAM_TAPE_H_
#define PROGRAM_TAPE_H_

#include "lang/lexer/token.h"
#include "program/instruction.h"

typedef struct {
  // TODO: Something that points to the file, func, etc...
  int32_t line, col;
  const Token *token;
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

// **********************
// Specialized functions.
// **********************
int tape_ins(Tape *tape, Op op, const Token *token);
int tape_ins_text(Tape *tape, Op op, const char text[], const Token *token);
int tape_ins_int(Tape *tape, Op op, int val, const Token *token);
int tape_ins_no_arg(Tape *tape, Op op, const Token *token);
int tape_ins_anon(Tape *tape, Op op, const Token *token);
int tape_ins_neg(Tape *tape, Op op, const Token *token);

int tape_label(Tape *tape, const Token *token);
// int tape_label_text(Tape *tape, const char text[]);
int tape_anon_label(Tape *tape, const Token *token);
// int tape_function_with_args(Tape *tape, const Token *token, Q *args);
// int tape_anon_function_with_args(Tape *tape, const Token *token, Q *args);

int tape_module(Tape *tape, const Token *token);
int tape_class(Tape *tape, const Token *token);
int tape_class_with_parents(Tape *tape, const Token *token, Q *tokens);
int tape_endclass(Tape *tape, const Token *token);

#endif /* PROGRAM_TAPE_H_ */