// deserialize.h
//
// Created on: Nov 10, 2017
//     Author: Jeff

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_SERIALIZATION_DESERIALIZE_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_SERIALIZATION_DESERIALIZE_H_

#include <stdint.h>
#include <stdio.h>

#include "zinnia/program/instruction.h"
#include "zinnia/util/void_array.h"

#define deserialize_type(file, type, p) \
  deserialize_bytes(file, sizeof(type), ((char *)p), sizeof(type))

int deserialize_bytes(FILE *file, uint32_t num_bytes, char *buffer,
                      uint32_t buffer_sz);
int deserialize_string(FILE *file, char *buffer, uint32_t buffer_sz);
int deserialize_ins(FILE *file, const CharPtrArray *strings, Instruction *ins);

#endif
/* COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_SERIALIZATION_DESERIALIZE_H_ */