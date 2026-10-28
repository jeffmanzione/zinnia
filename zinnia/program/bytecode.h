// instruction.h
//
// Created on: Jun 6, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_BYTECODE_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_BYTECODE_H_

#include <stdbool.h>

#include "c-data-structures/arraylike.h"
#include "c-data-structures/stable_maplike.h"
#include "file-utils/file_info.h"
#include "language-tools/lexer/token.h"
#include "zinnia/program/instruction.h"
#include "zinnia/util/error.h"
#include "zinnia/util/void_array.h"

typedef struct {
  // TODO: Something that points to the file, func, etc...
  int32_t line, col;
  const Token *token;
  // Only set if there is another source file that generated the file read.
  int32_t source_line, source_col;
  const Token *source_token;
} SourceMapping;

DEFINE_ARRAYLIKE(SourceMappingArray, SourceMapping);

typedef struct {
  const char *name;
  uint32_t index;
  bool is_const, is_async;
} FunctionRef;

DEFINE_STABLE_MAPLIKE(FunctionRefMap, char *, FunctionRef);

typedef struct {
  const char *name;
} FieldRef;

DEFINE_STABLE_MAPLIKE(FieldRefMap, char *, FieldRef);

typedef struct {
  const char *name;
  uint32_t start_index;  // inclusive
  uint32_t end_index;    // exclusive
  FunctionRefMap func_refs;
  FieldRefMap field_refs;
  CharPtrArray supers;
} ClassRef;

DEFINE_STABLE_MAPLIKE(ClassRefMap, char *, ClassRef);
DEFINE_ARRAYLIKE(ClassRefPtrArray, ClassRef *);

typedef struct Bytecode_ Bytecode;

// Access-related functions.
const Instruction *bytecode_get(const Bytecode *bytecode, uint32_t index);
Instruction *bytecode_get_mutable(Bytecode *bytecode, uint32_t index);
const SourceMapping *bytecode_get_source(const Bytecode *bytecode,
                                         uint32_t index);
size_t bytecode_size(const Bytecode *bytecode);

uint32_t bytecode_class_count(const Bytecode *bytecode);
ClassRefMapIOIterator bytecode_classes(const Bytecode *bytecode);

uint32_t bytecode_func_count(const Bytecode *bytecode);
FunctionRefMapIOIterator bytecode_functions(const Bytecode *bytecode);

const char *bytecode_module_name(const Bytecode *bytecode);

// Build-related functions.
Bytecode *bytecode_create();
void bytecode_delete(Bytecode *bytecode);
Instruction *bytecode_add(Bytecode *bytecode);
SourceMapping *bytecode_add_source(Bytecode *bytecode, Instruction *ins);

void bytecode_start_func_at_index(Bytecode *bytecode, const char name[],
                                  uint32_t index, bool is_async);
void bytecode_start_class(Bytecode *bytecode, const char name[]);
void bytecode_end_class(Bytecode *bytecode);
void bytecode_field(Bytecode *bytecode, const char *field);
ClassRef *bytecode_start_class_at_index(Bytecode *bytecode, const char name[],
                                        uint32_t index);
void bytecode_end_class_at_index(Bytecode *bytecode, uint32_t index);

void bytecode_append(Bytecode *head, Bytecode *tail);

void bytecode_write(const Bytecode *bytecode, FILE *file, bool minimize);
void bytecode_read(Bytecode *const bytecode, TokenArray *tokens);

void bytecode_set_external_source(Bytecode *const bytecode,
                                  const char file_name[]);
const char *bytecode_get_external_source(const Bytecode *const bytecode);

void bytecode_set_body(Bytecode *const bytecode, FileInfo *fi);
const char *bytecode_get_sourceline(const Bytecode *const bytecode, int line);

// **********************
// Specialized functions.
// **********************
int bytecode_ins_raw(Bytecode *bytecode, Instruction *ins);
int bytecode_ins(Bytecode *bytecode, Op op, const Token *token);
int bytecode_ins_text(Bytecode *bytecode, Op op, const char text[],
                      const Token *token);

int bytecode_ins_int(Bytecode *bytecode, Op op, int val, const Token *token);

// int tape_ins_int(Tape *tape, Op op, int val, const Token *token);
int bytecode_ins_no_arg(Bytecode *bytecode, Op op, const Token *token);
int bytecode_ins_anon(Bytecode *bytecode, Op op, const Token *token);
int bytecode_ins_neg(Bytecode *bytecode, Op op, const Token *token);

int bytecode_label(Bytecode *bytecode, const Token *token);
int bytecode_label_async(Bytecode *bytecode, const Token *token);
int bytecode_label_text(Bytecode *bytecode, const char text[]);
int bytecode_label_text_async(Bytecode *bytecode, const char text[]);
int bytecode_anon_label(Bytecode *bytecode, const Token *token);
int bytecode_anon_label_async(Bytecode *bytecode, const Token *token);

int bytecode_module(Bytecode *bytecode, const Token *token);
int bytecode_class(Bytecode *bytecode, const Token *token);
const ClassRef *bytecode_get_class(const Bytecode *bytecode,
                                   const char class_name[]);
int bytecode_class_with_parents(Bytecode *bytecode, const Token *token,
                                CharPtrArray *tokens);
int bytecode_endclass(Bytecode *bytecode, const Token *token);

Primitive token_to_primitive(const Token *tok);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_BYTECODE_H_ */