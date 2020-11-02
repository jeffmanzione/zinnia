// serialize.h
//
// Created on: Jul 21, 2017
//     Author: Jeff

#ifndef PROGRAM_SERIALIZATION_SERIALIZE_H_
#define PROGRAM_SERIALIZATION_SERIALIZE_H_

#include "entity/primitive.h"
#include "program/instruction.h"
#include "program/serialization/buffer.h"
#include "struct/map.h"

#define serialize_type(buffer, type, val)                                      \
  serialize_bytes(buffer, ((char *)(&(val))), sizeof(type))

// Returns number of chars written
int serialize_bytes(WBuffer *buffer, const char *start, int num_bytes);
int serialize_primitive(WBuffer *buffer, Primitive val);
int serialize_str(WBuffer *buffer, const char *str);
int serialize_ins(WBuffer *const buffer, const Instruction *c,
                  const Map *const string_index);

#endif /* PROGRAM_SERIALIZATION_SERIALIZE_H_ */