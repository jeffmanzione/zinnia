// op.c
//
// Created on: Jun 4, 2020
//     Author: Jeff Manzione

#include "program/op.h"

static const char *_op_strs[] = {
    "nop", "res", "let", "assn", "set", "get", "call", "clln", "ret",
};

inline const char *op_to_str(Op op) { return _op_strs[op]; }