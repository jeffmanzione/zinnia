// deserialize.c
//
// Created on: Nov 10, 2017
//     Author: Jeff

#include "program/serialization/deserialize.h"

#include "debug/debug.h"
#include "entity/primitive.h"
#include "program/instruction.h"

#define deserialize_type(file, type, p)                                        \
  deserialize_bytes(file, sizeof(type), ((char *)p), sizeof(type))

int deserialize_bytes(FILE *file, uint32_t num_bytes, char *buffer,
                      uint32_t buffer_sz) {
  ASSERT(NOT_NULL(file), NOT_NULL(buffer));
  if (num_bytes > buffer_sz) {
    return 0;
  }
  return fread(buffer, sizeof(char), num_bytes, file);
}

int deserialize_string(FILE *file, char *buffer, uint32_t buffer_sz) {
  char *start = buffer;

  do {
    if (!fread(buffer, sizeof(char), 1, file)) {
      *buffer = '\0';
    }
  } while ('\0' != *(buffer++));
  return buffer - start;
}

int deserialize_val(FILE *file, Primitive *val) {
  int i = 0;
  uint8_t val_type;

  int32_t int_val;
  float float_val;
  int8_t char_val;

  i += deserialize_type(file, uint8_t, &val_type);
  switch (val_type) {
  case PRIMITIVE_INT:
    i += deserialize_type(file, int64_t, &int_val);
    *val = primitive_int(int_val);
    break;
  case PRIMITIVE_FLOAT:
    i += deserialize_type(file, double, &float_val);
    *val = primitive_float(float_val);
    break;
  case PRIMITIVE_CHAR:
  default:
    i += deserialize_type(file, int8_t, &char_val);
    *val = primitive_char(char_val);
  }
  return i;
}

int deserialize_ins(FILE *file, const AList *const strings, Instruction *ins) {
  int i = 0;
  uint8_t op;
  uint8_t param;
  i += deserialize_type(file, uint8_t, &op);
  i += deserialize_type(file, uint8_t, &param);
  ins->op = op;
  ins->type = param;
  uint16_t ref16;
  uint8_t ref8;
  switch (param) {
  case INSTRUCTION_PRIMITIVE:
    i += deserialize_val(file, &ins->val);
    break;
  case INSTRUCTION_ID:
  case INSTRUCTION_STRING:
    if (alist_len(strings) > UINT8_MAX) {
      i += deserialize_type(file, uint16_t, &ref16);
    } else {
      i += deserialize_type(file, uint8_t, &ref8);
      ref16 = (uint16_t)ref8;
    }
    char *str = *((char **)alist_get(strings, (uint32_t)ref16));
    if (INSTRUCTION_ID == param) {
      ins->id = str;
    } else {
      ins->str = str;
    }
    break;
  case INSTRUCTION_NO_ARG:
  default:
    break;
  }
  return i;
}