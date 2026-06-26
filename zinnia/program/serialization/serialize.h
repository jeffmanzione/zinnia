// serialize.h
//
// Created on: Jul 21, 2017
//     Author: Jeff

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_SERIALIZATION_SERIALIZE_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_SERIALIZATION_SERIALIZE_H_

#include "c-data-structures/maplike.h"
#include "zinnia/entity/primitive.h"
#include "zinnia/program/instruction.h"
#include "zinnia/program/serialization/buffer.h"

DEFINE_MAPLIKE(StringIndexMap, char *, int);

#define serialize_type(buffer, type, val) \
  serialize_bytes(buffer, ((char *)(&(val))), sizeof(type))

// Returns number of chars written
int serialize_bytes(WBuffer *buffer, const char *start, int num_bytes);
int serialize_primitive(WBuffer *buffer, Primitive val);
int serialize_str(WBuffer *buffer, const char *str);
int serialize_ins(WBuffer *const buffer, const Instruction *c,
                  const StringIndexMap *string_index);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_SERIALIZATION_SERIALIZE_H_ */