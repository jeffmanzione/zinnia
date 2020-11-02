// instruction.h
//
// Created on: Jun 6, 2020
//     Author: Jeff Manzione

#ifndef PROGRAM_TAPE_H_
#define PROGRAM_TAPE_H_

#include "debug/debug.h"
#include "lang/lexer/token.h"
#include "program/instruction.h"
#include "struct/keyed_list.h"
#include "struct/q.h"

typedef struct {
  // TODO: Something that points to the file, func, etc...
  int32_t line, col;
  const Token *token;
} SourceMapping;

typedef struct {
  const char *name;
  uint32_t index;
} FunctionRef;

typedef struct {
  const char *name;
  uint32_t start_index; // inclusive
  uint32_t end_index;   // exclusive
  KeyedList func_refs;
  AList supers;
} ClassRef;

typedef struct _Tape Tape;

// Access-related functions.
const Instruction *tape_get(const Tape *tape, uint32_t index);
Instruction *tape_get_mutable(Tape *tape, uint32_t index);
const SourceMapping *tape_get_source(const Tape *tape, uint32_t index);
size_t tape_size(const Tape *tape);

uint32_t tape_class_count(const Tape *tape);
KL_iter tape_classes(const Tape *tape);

uint32_t tape_func_count(const Tape *tape);
KL_iter tape_functions(const Tape *tape);

const char *tape_module_name(const Tape *tape);

// Build-related functions.
Tape *tape_create();
void tape_delete(Tape *tape);
Instruction *tape_add(Tape *tape);
SourceMapping *tape_add_source(Tape *tape, Instruction *ins);

void tape_start_func(Tape *tape, const char name[]);
void tape_start_func_at_index(Tape *tape, const char name[], uint32_t index);
void tape_start_class(Tape *tape, const char name[]);
void tape_end_class(Tape *tape);
ClassRef *tape_start_class_at_index(Tape *tape, const char name[],
                                    uint32_t index);
void tape_end_class_at_index(Tape *tape, uint32_t index);

void tape_append(Tape *head, Tape *tail);

void tape_write(const Tape *tape, FILE *file);
void tape_read(Tape *const tape, Q *tokens);

// **********************
// Specialized functions.
// **********************
int tape_ins_raw(Tape *tape, Instruction *ins);
int tape_ins(Tape *tape, Op op, const Token *token);
int tape_ins_text(Tape *tape, Op op, const char text[], const Token *token);

DEB_FN(int, tape_ins_int, Tape *tape, Op op, int val, const Token *token);
#define tape_ins_int(tape, op, val, token)                                     \
  CALL_FN(tape_ins_int__, tape, op, val, token)

// int tape_ins_int(Tape *tape, Op op, int val, const Token *token);
int tape_ins_no_arg(Tape *tape, Op op, const Token *token);
int tape_ins_anon(Tape *tape, Op op, const Token *token);
int tape_ins_neg(Tape *tape, Op op, const Token *token);

int tape_label(Tape *tape, const Token *token);
int tape_label_text(Tape *tape, const char text[]);
int tape_anon_label(Tape *tape, const Token *token);

int tape_module(Tape *tape, const Token *token);
int tape_class(Tape *tape, const Token *token);
int tape_class_with_parents(Tape *tape, const Token *token, Q *tokens);
int tape_endclass(Tape *tape, const Token *token);

#endif /* PROGRAM_TAPE_H_ */