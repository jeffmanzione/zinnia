#include "program/instruction.h"

#include <stdio.h>

#include "debug/debug.h"
#define OP_FMT "  %-6s"
#define STR_FMT "%s"
#define INT_FMT "%d"
#define FLT_FMT "%f"
#define OP_NO_ARG_FMT "  %s"

int _instruction_write_primitive(const Instruction *ins, FILE *file);

int instruction_write(const Instruction *ins, FILE *file) {
  switch (ins->type) {
    case INSTRUCTION_NO_ARG:
      return fprintf(file, OP_NO_ARG_FMT, op_to_str(ins->op));
    case INSTRUCTION_ID:
      return fprintf(file, OP_FMT STR_FMT, op_to_str(ins->op), ins->id);
    case INSTRUCTION_STRING:
      return fprintf(file, OP_FMT STR_FMT, op_to_str(ins->op), ins->str);
    case INSTRUCTION_PRIMITIVE:
      return _instruction_write_primitive(ins, file);
    default:
      ERROR("Unknown instruction type.");
      return -1;
  }
}

int _instruction_write_primitive(const Instruction *ins, FILE *file) {
  switch (ptype(&ins->val)) {
    case INT:
      return fprintf(file, OP_FMT INT_FMT, op_to_str(ins->op), pint(&ins->val));
    case FLOAT:
      return fprintf(file, OP_FMT FLT_FMT, op_to_str(ins->op),
                     pfloat(&ins->val));
    default:
      ERROR("Unkown primitive instruction.");
      return -1;
  }
}