// tape_binary.h
//
// Created on: Nov 1, 2020
//     Author: Jeff

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_TAPE_BINARY_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_TAPE_BINARY_H_

#include <stdio.h>

#include "zinnia/program/tape.h"

void tape_read_binary(Tape *const tape, FILE *file);
void tape_write_binary(const Tape *const tape, FILE *file);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_TAPE_BINARY_H_ */