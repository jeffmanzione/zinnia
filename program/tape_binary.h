// tape_binary.h
//
// Created on: Nov 1, 2020
//     Author: Jeff

#ifndef PROGRAM_TAPE_BINARY_H_
#define PROGRAM_TAPE_BINARY_H_

#include <stdio.h>

#include "program/tape.h"

void tape_read_binary(Tape *const tape, FILE *file);
void tape_write_binary(const Tape *const tape, FILE *file);

#endif /* PROGRAM_TAPE_BINARY_H_ */