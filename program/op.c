// op.c
//
// Created on: Jun 4, 2020
//     Author: Jeff Manzione

#include "program/op.h"

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