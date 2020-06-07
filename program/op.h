// op.h
//
// Created on: Jun 4, 2020
//     Author: Jeff Manzione

#ifndef PROGRAM_OP_H_
#define PROGRAM_OP_H_

typedef enum {
  // Do nothing.
  NOP,
  // Loads something into resval.
  RES,
  // Assigns a value locally.
  LET,
  // Assigns a value looking up the hierarchy for a variable with the given
  // name.
  ASSN,
  // Field setter.
  SET,
  // Field getter.
  GET,
  // Calls a function with arguments.
  CALL,
  // Calls a function with no arguments.
  CLLN,
  // Returns to the previous caller.
  RET,
} Op;

const char *op_to_str(Op op);

#endif /* PROGRAM_OP_H_ */