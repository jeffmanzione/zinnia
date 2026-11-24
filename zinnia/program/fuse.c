// fuse.c
//
// Instructions the VM runs together, as one.

#include "zinnia/program/fuse.h"

#include <stdbool.h>

#include "zinnia/program/instruction.h"
#include "zinnia/program/op.h"

static bool is_comparison_(Op op) {
  switch (op) {
    case LT:
    case GT:
    case LTE:
    case GTE:
    case EQ:
    case NEQ:
      return true;
    default:
      return false;
  }
}

// Instructions that always go on to the next one, unless they raise an error.
static bool goes_on_(Op op) {
  switch (op) {
    case INC:
    case DEC:
    case SET:
    case ADD:
    case SUB:
    case MULT:
    case DIV:
    case MOD:
      return true;
    default:
      return false;
  }
}

// Whether the ARGC at [i] is followed by the instructions that move its
// arguments into slots: IFN, then RES and LET for each argument, then JMP.
static bool is_args_prologue_(const Bytecode *bytecode, uint32_t i) {
  const Instruction *argc = bytecode_get(bytecode, i);
  if (INSTRUCTION_PRIMITIVE != argc->type) {
    return false;
  }
  const int64_t n = pint(&argc->val);
  const int64_t end = (int64_t)i + 2 + 2 * n;  // The JMP.
  if (n < 1 || end >= (int64_t)bytecode_size(bytecode)) {
    return false;
  }
  const Instruction *ifn = bytecode_get(bytecode, i + 1);
  if (IFN != ifn->op || INSTRUCTION_PRIMITIVE != ifn->type) {
    return false;
  }
  for (int64_t k = 0; k < n; ++k) {
    const Instruction *res = bytecode_get(bytecode, i + 2 + 2 * k);
    const Instruction *let = bytecode_get(bytecode, i + 3 + 2 * k);
    if (RES != res->op || INSTRUCTION_NO_ARG != res->type || LET != let->op ||
        INSTRUCTION_ID != let->type || 0 == let->slot) {
      return false;
    }
  }
  const Instruction *jmp = bytecode_get(bytecode, end);
  return JMP == jmp->op && INSTRUCTION_PRIMITIVE == jmp->type;
}

static Fuse fuse_for_(const Bytecode *bytecode, uint32_t i) {
  const Instruction *ins = bytecode_get(bytecode, i);
  const Instruction *next = bytecode_get(bytecode, i + 1);
  if (ARGC == ins->op) {
    return is_args_prologue_(bytecode, i) ? FUSE_ARGS : FUSE_NONE;
  }
  if (is_comparison_(ins->op) && (IF == next->op || IFN == next->op) &&
      INSTRUCTION_PRIMITIVE == next->type) {
    return FUSE_BRANCH;
  }
  if (goes_on_(ins->op) && JMP == next->op &&
      INSTRUCTION_PRIMITIVE == next->type) {
    return FUSE_JMP;
  }
  // A function of one parameter receives it in acc.
  if (SLOTS == ins->op && LET == next->op && INSTRUCTION_ID == next->type &&
      0 != next->slot) {
    return FUSE_LET;
  }
  return FUSE_NONE;
}

void bytecode_fuse(Bytecode *bytecode, uint32_t start) {
  const uint32_t size = bytecode_size(bytecode);
  for (uint32_t i = start; i < size; ++i) {
    bytecode_get_mutable(bytecode, i)->fuse =
        (i + 1 < size) ? fuse_for_(bytecode, i) : FUSE_NONE;
  }
}
