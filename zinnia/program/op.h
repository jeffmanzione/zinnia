// op.h
//
// Created on: Jun 4, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_OP_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_OP_H_

typedef enum {
  // Do nothing.
  NOP,
  EXIT,
  // Loads something into resval.
  RES,
  TGET,
  TLEN,
  SET,
  MSET,
  // Assigns a value locally.
  LET,
  PUSH,
  PEEK,
  PSRS,  // PUSH+RES
  NOT,   // where !1 == Nil
  NOTC,  // C-like NOT, where !1 == 0
  GT,
  LT,
  EQ,
  NEQ,
  GTE,
  LTE,
  AND,
  OR,
  XOR,
  BAND,
  BOR,
  BXOR,
  IF,
  IFN,
  JMP,
  NBLK,
  BBLK,
  RET,
  ADD,
  SUB,
  MULT,
  DIV,
  MOD,
  INC,
  DEC,
  FINC,
  FDEC,
  SINC,
  CALL,
  CLLN,
  TUPL,
  TGTE,
  TLTE,
  TEQ,
  DUP,
  GOTO,
  PRNT,
  LMDL,
  GET,
  GTSH,  // GET+PUSH
  RNIL,  // RES Nil
  PNIL,  // PUSH Nil
  FLD,
  FLDC,
  IS,
  ADR,
  RAIS,
  CTCH,
  // Arrays
  ANEW,
  AIDX,
  ASET,
  // Const
  CNST,
  SETC,
  LETC,
  SGET,
  // Async
  WAIT,
  // True/False
  RTRU,
  RFLS,
  PTRU,
  PFLS,
  // Immutable
  IRES,
  IPSH,
  // NOT A REAL OP
  OP_BOUND,
} Op;

const char *op_to_str(Op op);
Op str_to_op(const char op_str[]);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_OP_H_ */