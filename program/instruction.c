#include "program/instruction.h"

#include <inttypes.h>
#include <stdio.h>

#include "alloc/alloc.h"
#include "debug/debug.h"
#include "util/string_util.h"

#define OP_FMT "  %-6s"
#define ID_FMT "%s"
#define STR_FMT "\'%.*s\'"
#define INT_FMT "%" PRId64
#define FLT_FMT "%f"
#define OP_NO_ARG_FMT "  %s"

int _instruction_write_primitive(const Instruction *ins, FILE *file);

int instruction_write(const Instruction *ins, FILE *file) {
  char *tmp;
  int num = 0;
  switch (ins->type) {
  case INSTRUCTION_NO_ARG:
    return fprintf(file, OP_NO_ARG_FMT, op_to_str(ins->op));
  case INSTRUCTION_ID:
    return fprintf(file, OP_FMT ID_FMT, op_to_str(ins->op), ins->id);
  case INSTRUCTION_STRING:
    tmp = escape(ins->str);
    num = fprintf(file, OP_FMT STR_FMT, op_to_str(ins->op), (int)(strlen(tmp)),
                  tmp);
    RELEASE(tmp);
    return num;
  case INSTRUCTION_PRIMITIVE:
    return _instruction_write_primitive(ins, file);
  default:
    FATALF("Unknown instruction type.");
    return -1;
  }
}

int _instruction_write_primitive(const Instruction *ins, FILE *file) {
  switch (ptype(&ins->val)) {
  case PRIMITIVE_INT:
    return fprintf(file, OP_FMT INT_FMT, op_to_str(ins->op), pint(&ins->val));
  case PRIMITIVE_FLOAT:
    return fprintf(file, OP_FMT FLT_FMT, op_to_str(ins->op), pfloat(&ins->val));
  default:
    FATALF("Unkown primitive instruction.");
    return -1;
  }
}