// bytecode_binary.h
//
// Created on: Nov 1, 2020
//     Author: Jeff

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_BYTECODE_BINARY_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_BYTECODE_BINARY_H_

#include <stdio.h>

#include "zinnia/program/bytecode.h"

void bytecode_read_binary(Bytecode *const bytecode, FILE *file);
void bytecode_write_binary(const Bytecode *const bytecode, FILE *file);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_BYTECODE_BINARY_H_ */