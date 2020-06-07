#include "program/instruction.h"

#include <stdio.h>

#include "debug/debug.h"

#define OP_ID_FMT "  %-6s%s"
#define OP_STR_FMT "  %-6s%s"
#define OP_INT_FMT "  %-6s%d"
#define OP_FLT_FMT "  %-6s%f"
#define OP_NO_ARG_FMT "  %s"

void _instruction_write_primitive(const Instruction *ins, FILE *file);

void instruction_write(const Instruction *ins, FILE *file) {
  switch (ins->type) {
    case INSTRUCTION_NO_ARG:
      fprintf(file, OP_NO_ARG_FMT, op_to_str(ins->op));
      return;
    case INSTRUCTION_ID:
      fprintf(file, OP_ID_FMT, op_to_str(ins->op), ins->id);
      return;
    case INSTRUCTION_STRING:
      fprintf(file, OP_STR_FMT, op_to_str(ins->op), ins->str);
      return;
    case INSTRUCTION_PRIMITIVE:
      _instruction_write_primitive(ins, file);
      return;
    default:
      ERROR("Unknown instruction type.");
  }
}

void _instruction_write_primitive(const Instruction *ins, FILE *file) {
  switch (ptype(&ins->val)) {
    case INT:
      fprintf(file, OP_INT_FMT, op_to_str(ins->op), pint(&ins->val));
      return;
    case FLOAT:
      fprintf(file, OP_FLT_FMT, op_to_str(ins->op), pfloat(&ins->val));
      return;
    default:
      ERROR("Unkown primitive instruction.");
  }
}