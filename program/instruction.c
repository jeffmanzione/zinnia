#include "program/instruction.h"

#include <inttypes.h>
#include <stdio.h>

#include "alloc/alloc.h"
#include "debug/debug.h"
#include "util/string_util.h"

#define INSTRUCTION_INDENT "  "
#define OP_FMT(minimize) ((minimize) ? "%s " : "%-5s ")
#define ID_FMT "%s"
#define STR_FMT "\'%.*s\'"
#define INT_FMT "%" PRId64
#define FLT_FMT "%f"
#define OP_NO_ARG_FMT "%s"

int _instruction_write_primitive(const Instruction *ins, FILE *file,
                                 bool minimize);

int instruction_write(const Instruction *ins, FILE *file, bool minimize) {
  int chars_written = 0;
  if (!minimize) {
    chars_written += fprintf(file, "%s", INSTRUCTION_INDENT);
  }
  char *tmp;
  int num = 0;
  switch (ins->type) {
  case INSTRUCTION_NO_ARG:
    return chars_written + fprintf(file, OP_NO_ARG_FMT, op_to_str(ins->op));
  case INSTRUCTION_ID:
    return chars_written + fprintf(file, OP_FMT(minimize), op_to_str(ins->op)) +
           fprintf(file, ID_FMT, ins->id);
  case INSTRUCTION_STRING:
    tmp = escape(ins->str);
    num = chars_written + fprintf(file, OP_FMT(minimize), op_to_str(ins->op)) +
          fprintf(file, STR_FMT, (int)(strlen(tmp)), tmp);
    RELEASE(tmp);
    return num;
  case INSTRUCTION_PRIMITIVE:
    return chars_written + _instruction_write_primitive(ins, file, minimize);
  default:
    FATALF("Unknown instruction type.");
    return -1;
  }
}

int _instruction_write_primitive(const Instruction *ins, FILE *file,
                                 bool minimize) {
  int chars_written = fprintf(file, OP_FMT(minimize), op_to_str(ins->op));
  switch (ptype(&ins->val)) {
  case PRIMITIVE_INT:
    return chars_written + fprintf(file, INT_FMT, pint(&ins->val));
  case PRIMITIVE_FLOAT:
    return chars_written + fprintf(file, FLT_FMT, pfloat(&ins->val));
  default:
    FATALF("Unkown primitive instruction.");
    return -1;
  }
}