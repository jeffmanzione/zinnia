// op.c
//
// Created on: Jun 4, 2020
//     Author: Jeff Manzione

#include "program/op.h"

#include <string.h>

static const char *_op_strs[] = {
    "nop",  "exit", "res",  "tget", "tlen", "set",  "let",  "push",
    "peek", "psrs", "not",  "notc", "gt",   "lt",   "eq",   "neq",
    "gte",  "lte",  "and",  "or",   "xor",  "if",   "ifn",  "jmp",
    "nblk", "bblk", "ret",  "add",  "sub",  "mult", "div",  "mod",
    "inc",  "dec",  "finc", "fdec", "sinc", "call", "clln", "tupl",
    "tgte", "tlte", "teq",  "dup",  "goto", "prnt", "lmdl", "get",
    "gtsh", "rnil", "pnil", "fld",  "fldc", "is",   "adr",  "rais",
    "ctch", "anew", "aidx", "aset", "cnst", "setc", "letc", "sget"};

inline const char *op_to_str(Op op) { return _op_strs[op]; }

Op str_to_op(const char op_str[]) {
  int i;
  for (i = 0; i < sizeof(_op_strs); ++i) {
    if (0 == strcmp(op_str, _op_strs[i])) {
      return i;
    }
  }
  return -1;
}