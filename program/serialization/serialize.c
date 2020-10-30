/*
 * serialize.c
 *
 *  Created on: Jul 21, 2017
 *      Author: Jeff
 */

#include "program/serialization/serialize.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "debug/debug.h"
#include "program/instruction.h"

int serialize_bytes(WBuffer *buffer, const char *start, int num_bytes) {
  buffer_write(buffer, start, num_bytes);
  return num_bytes;
}

int serialize_primitive(WBuffer *buffer, Primitive val) {
  int32_t int_val;
  float float_val;
  int8_t char_val;

  int i = 0;
  uint8_t val_type = (uint8_t)ptype(&val);
  i += serialize_type(buffer, uint8_t, val_type);
  switch (val_type) {
  case INT:
    int_val = pint(&val);
    i += serialize_type(buffer, int64_t, int_val);
    break;
  case FLOAT:
    float_val = pfloat(&val);
    i += serialize_type(buffer, double, float_val);
    break;
  case CHAR:
  default:
    char_val = pchar(&val);
    i += serialize_type(buffer, int8_t, char_val);
  }
  return i;
}

int serialize_str(WBuffer *buffer, const char *str) {
  return serialize_bytes(buffer, str, strlen(str) + 1);
}

int serialize_ins(WBuffer *const buffer, const Instruction *ins,
                  const Map *const string_index) {
  bool use_short = map_size(string_index) > UINT8_MAX ? true : false;
  int i = 0;
  uint8_t op = (uint8_t)ins->op;
  uint8_t param = (uint8_t)ins->type;
  i += serialize_type(buffer, uint8_t, op);
  i += serialize_type(buffer, uint8_t, param);
  //  i += serialize_type(buffer, uint16_t, ins->row);
  //  i += serialize_type(buffer, uint16_t, ins->col);
  uint16_t ref;
  switch (ins->type) {
  case INSTRUCTION_PRIMITIVE:
    i += serialize_primitive(buffer, ins->val);
    break;
  case INSTRUCTION_STRING:
    ref = (uint16_t)(uintptr_t)map_lookup(string_index, ins->str);
    ASSERT(ref >= 0);
    if (use_short) {
      i += serialize_type(buffer, uint16_t, ref);
    } else {
      uint8_t ref_byte = (uint32_t)ref;
      i += serialize_type(buffer, uint8_t, ref_byte);
    }
    break;
  case INSTRUCTION_ID:
    ref = (uint16_t)(uintptr_t)map_lookup(string_index, ins->id);
    ASSERT(ref >= 0);
    if (use_short) {
      i += serialize_type(buffer, uint16_t, ref);
    } else {
      uint8_t ref_byte = (uint32_t)ref;
      i += serialize_type(buffer, uint8_t, ref_byte);
    }
    break;
  case INSTRUCTION_NO_ARG:
  default:
    break;
  }
  return i;
}