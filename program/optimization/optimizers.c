// optimizers.c
//
// Created on: Mar 3, 2018
//     Author: Jeff

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "alloc/arena/intern.h"
#include "entity/entity.h"
#include "program/instruction.h"
#include "program/optimization/optimizer.h"
#include "program/tape.h"
#include "struct/map.h"
#include "vm/intern.h"

Instruction _for_op(Op op) {
  Instruction ins = {.op = op};
  return ins;
}

bool _overwrites_res(Op op) {
  switch (op) {
  case RES:
    // case CLLN:
    return true;
  default:
    return false;
  }
  return false;
}

void optimizer_ResPush(OptimizeHelper *oh, const Tape *const tape, int start,
                       int end) {
  int i;
  for (i = start + 1; i < end; i++) {
    const Instruction *first = tape_get(tape, i - 1);
    const Instruction *second = tape_get(tape, i);
    if (RES == first->op && INSTRUCTION_NO_ARG != first->type &&
        PUSH == second->op && INSTRUCTION_NO_ARG == second->type &&
        NULL == map_lookup(&oh->i_gotos, as_ptr(i - 1))) {
      o_SetOp(oh, i - 1, PUSH);
      o_Remove(oh, i);
      ++i;
    }
  }
}

void optimizer_SetRes(OptimizeHelper *oh, const Tape *const tape, int start,
                      int end) {
  int i;
  for (i = start + 1; i < end; i++) {
    const Instruction *first = tape_get(tape, i - 1);
    const Instruction *second = tape_get(tape, i);
    if ((SET == first->op || LET == first->op) &&
        INSTRUCTION_ID == first->type && RES == second->op &&
        INSTRUCTION_ID == second->type &&
        first->id == second->id // same pointer because string interning
        && NULL == map_lookup(&oh->i_gotos, as_ptr(i - 1))) {
      o_Remove(oh, i);
    }
  }
}

void optimizer_SetPush(OptimizeHelper *oh, const Tape *const tape, int start,
                       int end) {
  int i;
  for (i = start + 1; i < end; i++) {
    const Instruction *first = tape_get(tape, i - 1);
    const Instruction *second = tape_get(tape, i);
    if ((SET == first->op || LET == first->op) &&
        INSTRUCTION_ID == first->type && PUSH == second->op &&
        INSTRUCTION_ID == second->type &&
        first->id == second->id // same pointer because string interning
        && NULL == map_lookup(&oh->i_gotos, as_ptr(i - 1))) {
      o_Replace(oh, i, _for_op(PUSH));
    }
  }
}

void optimizer_GetPush(OptimizeHelper *oh, const Tape *const tape, int start,
                       int end) {
  int i;
  for (i = start + 1; i < end; i++) {
    const Instruction *first = tape_get(tape, i - 1);
    const Instruction *second = tape_get(tape, i);
    if (GET == first->op && INSTRUCTION_NO_ARG != first->type &&
        PUSH == second->op && INSTRUCTION_NO_ARG == second->type &&
        NULL == map_lookup(&oh->i_gotos, as_ptr(i - 1))) {
      o_Remove(oh, i);
      o_SetOp(oh, i - 1, GTSH);
    }
  }
}

void optimizer_JmpRes(OptimizeHelper *oh, const Tape *const tape, int start,
                      int end) {
  int i;
  for (i = start + 1; i < end; i++) {
    const Instruction *first = tape_get(tape, i - 1);
    const Instruction *second = tape_get(tape, i);
    if (SET != first->op || JMP != second->op) {
      continue;
    }
    int32_t jmp_val = pint(&second->val);
    if (jmp_val >= 0) {
      continue;
    }
    const Instruction *jump_to_parent = tape_get(tape, i + jmp_val - 1);
    const Instruction *jump_to = tape_get(tape, i + jmp_val);
    if (SET != jump_to_parent->op || jump_to_parent->id != first->id ||
        RES != jump_to->op || INSTRUCTION_ID != jump_to->type ||
        first->id != jump_to->id) { // same pointer because string interning
      continue;
    }
    o_Remove(oh, i + jmp_val);
  }
}

void optimizer_PushRes(OptimizeHelper *oh, const Tape *const tape, int start,
                       int end) {
  int i;
  for (i = start + 1; i < end; i++) {
    const Instruction *first = tape_get(tape, i - 1);
    const Instruction *second = tape_get(tape, i);
    if (PUSH != first->op || RES != second->op || first->type != second->type ||
        NULL != map_lookup(&oh->i_gotos, as_ptr(i))) {
      continue;
    }
    if (first->type == INSTRUCTION_STRING) {
      if (first->str != second->str) {
        continue;
      }
    } else if (first->type == INSTRUCTION_ID) {
      if (first->str != second->str) {
        continue;
      }
    } else if (first->type == INSTRUCTION_PRIMITIVE) {
      if (!primitive_equals(&first->val, &second->val)) {
        continue;
      }
    }
    o_SetOp(oh, i - 1, PSRS);
    o_Remove(oh, i);
  }
}

void optimizer_ResPush2(OptimizeHelper *oh, const Tape *const tape, int start,
                        int end) {
  int i;
  for (i = start + 1; i < end; i++) {
    const Instruction *first = tape_get(tape, i - 1);
    const Instruction *second = tape_get(tape, i);
    if (RES == first->op && INSTRUCTION_NO_ARG == first->type &&
        PUSH == second->op && INSTRUCTION_NO_ARG == second->type &&
        NULL == map_lookup(&oh->i_gotos, as_ptr(i - 1))) {
      o_SetOp(oh, i - 1, PEEK);
      o_Remove(oh, i);
    }
  }
}

void optimizer_RetRet(OptimizeHelper *oh, const Tape *const tape, int start,
                      int end) {
  int i;
  for (i = start + 1; i < end; i++) {
    const Instruction *first = tape_get(tape, i - 1);
    const Instruction *second = tape_get(tape, i);
    if (RET == first->op && INSTRUCTION_NO_ARG == first->type &&
        RET == second->op && INSTRUCTION_NO_ARG == second->type &&
        NULL == map_lookup(&oh->i_gotos, as_ptr(i - 1))) {
      o_Remove(oh, i);
    }
  }
}

void optimizer_PeekRes(OptimizeHelper *oh, const Tape *const tape, int start,
                       int end) {
  int i;
  for (i = start + 1; i < end; i++) {
    const Instruction *first = tape_get(tape, i - 1);
    const Instruction *second = tape_get(tape, i);
    if (PEEK == first->op && (RES == second->op || TLEN == second->op) &&
        NULL == map_lookup(&oh->i_gotos, as_ptr(i - 1))) {
      o_Remove(oh, i - 1);
    }
  }
}

void optimizer_PeekPush(OptimizeHelper *oh, const Tape *const tape, int start,
                        int end) {
  int i;
  for (i = start + 1; i < end; i++) {
    const Instruction *first = tape_get(tape, i - 1);
    const Instruction *second = tape_get(tape, i);
    if (PEEK == first->op && INSTRUCTION_NO_ARG == first->type &&
        PUSH == second->op && INSTRUCTION_NO_ARG == second->type &&
        NULL == map_lookup(&oh->i_gotos, as_ptr(i - 1))) {
      o_SetOp(oh, i - 1, DUP);
      o_Remove(oh, i);
    }
  }
}

void optimizer_GroupStatics(OptimizeHelper *oh, const Tape *const tape,
                            int start, int end) {}

void optimizer_Increment(OptimizeHelper *oh, const Tape *const tape, int start,
                         int end) {
  //  push  a
  //  push  1
  //  add
  //  set   a

  int i;
  for (i = start + 3; i < end; i++) {
    const Instruction *first = tape_get(tape, i - 3);
    const Instruction *second = tape_get(tape, i - 2);
    const Instruction *third = tape_get(tape, i - 1);
    const Instruction *fourth = tape_get(tape, i);
    if (PUSH == first->op && INSTRUCTION_ID == first->type &&
        PUSH == second->op && INSTRUCTION_PRIMITIVE == second->type &&
        PRIMITIVE_INT == ptype(&second->val) && 1 == pint(&second->val) &&
        (ADD == third->op || SUB == third->op) && SET == fourth->op &&
        INSTRUCTION_ID == fourth->type && first->id == fourth->id &&
        NULL == map_lookup(&oh->i_gotos, as_ptr(i)) &&
        NULL == map_lookup(&oh->i_gotos, as_ptr(i - 1)) &&
        NULL == map_lookup(&oh->i_gotos, as_ptr(i - 2)) &&
        NULL == map_lookup(&oh->i_gotos, as_ptr(i - 3))) {
      o_SetOp(oh, i - 3, ADD == third->op ? INC : DEC);
      o_Remove(oh, i - 2);
      o_Remove(oh, i - 1);
      o_Remove(oh, i);
    }
  }

  //  res   i
  //  add   1
  //  set   i

  for (i = start + 2; i < end; i++) {
    const Instruction *first = tape_get(tape, i - 2);
    const Instruction *second = tape_get(tape, i - 1);
    const Instruction *third = tape_get(tape, i);
    if (RES == first->op && INSTRUCTION_ID == first->type &&
        (ADD == second->op || SUB == second->op) &&
        INSTRUCTION_PRIMITIVE == second->type &&
        PRIMITIVE_INT == ptype(&second->val) && 1 == pint(&second->val) &&
        SET == third->op && INSTRUCTION_ID == third->type &&
        first->id == third->id &&
        NULL == map_lookup(&oh->i_gotos, as_ptr(i - 1)) &&
        NULL == map_lookup(&oh->i_gotos, as_ptr(i - 2)) &&
        NULL == map_lookup(&oh->i_gotos, as_ptr(i - 3))) {
      o_SetOp(oh, i - 2, ADD == second->op ? INC : DEC);
      o_Remove(oh, i - 1);
      o_Remove(oh, i);
    }
  }
}

void optimizer_SetEmpty(OptimizeHelper *oh, const Tape *const tape, int start,
                        int end) {
  int i;
  for (i = start + 1; i < end; i++) {
    const Instruction *first = tape_get(tape, i - 1);
    const Instruction *second = tape_get(tape, i);
    if (TGET == first->op && INSTRUCTION_PRIMITIVE == first->type &&
        (SET == second->op || LET == second->op) &&
        INSTRUCTION_ID == second->type && 0 == strncmp(second->id, "_", 2) &&
        NULL == map_lookup(&oh->i_gotos, as_ptr(i - 1))) {
      o_Remove(oh, i - 1);
      o_Remove(oh, i);
    }
  }
}

void optimizer_PushResEmpty(OptimizeHelper *oh, const Tape *const tape,
                            int start, int end) {
  int i;
  for (i = start + 1; i < end; i++) {
    const Instruction *first = tape_get(tape, i - 1);
    const Instruction *second = tape_get(tape, i);
    if (PUSH == first->op && INSTRUCTION_NO_ARG == first->type &&
        RES == second->op && INSTRUCTION_NO_ARG == second->type &&
        NULL == map_lookup(&oh->i_gotos, as_ptr(i - 1))) {
      o_Remove(oh, i - 1);
      o_Remove(oh, i);
    }
  }
}

void optimizer_PeekPeek(OptimizeHelper *oh, const Tape *const tape, int start,
                        int end) {
  int i;
  for (i = start + 1; i < end; i++) {
    const Instruction *first = tape_get(tape, i - 1);
    const Instruction *second = tape_get(tape, i);
    if (PEEK == first->op && INSTRUCTION_NO_ARG == first->type &&
        PEEK == second->op && INSTRUCTION_NO_ARG == second->type &&
        NULL == map_lookup(&oh->i_gotos, as_ptr(i - 1))) {
      o_Remove(oh, i - 1);
    }
  }
}

void optimizer_PushRes2(OptimizeHelper *oh, const Tape *const tape, int start,
                        int end) {
  int i;
  for (i = start + 1; i < end; i++) {
    const Instruction *first = tape_get(tape, i - 1);
    const Instruction *second = tape_get(tape, i);
    if (PUSH == first->op && RES == second->op && first->type == second->type &&
        first->type == INSTRUCTION_NO_ARG &&
        NULL == map_lookup(&oh->i_gotos, as_ptr(i))) {
      o_Remove(oh, i - 1);
      o_Remove(oh, i);
    }
  }
}

bool is_math_op(Op op) {
  switch (op) {
  case ADD:
  case SUB:
  case DIV:
  case MULT:
  case MOD:
  case LT:
  case LTE:
  case GTE:
  case GT:
  case EQ:
    return true;
  default:
    return false;
  }
}

// Run after ResPush.
// Consider allowing second param to be ID. Would need to add to
// execute_INSTRUCTION_ID.
void optimizer_SimpleMath(OptimizeHelper *oh, const Tape *const tape, int start,
                          int end) {
  int i;
  for (i = start + 2; i < end; i++) {
    const Instruction *first = tape_get(tape, i - 2);
    const Instruction *second = tape_get(tape, i - 1);
    const Instruction *third = tape_get(tape, i);
    if (PUSH == first->op && PUSH == second->op && is_math_op(third->op) &&
        (second->type == INSTRUCTION_PRIMITIVE ||
         second->type == INSTRUCTION_ID) &&
        NULL == map_lookup(&oh->i_gotos, as_ptr(i)) &&
        NULL == map_lookup(&oh->i_gotos, as_ptr(i - 1))) {
      if (first->type == INSTRUCTION_NO_ARG) {
        o_Remove(oh, i - 2);
      } else {
        o_SetOp(oh, i - 2, RES);
      }
      o_SetOp(oh, i - 1, third->op);
      o_Remove(oh, i);
    }
  }
}

void optimizer_Nil(OptimizeHelper *oh, const Tape *const tape, int start,
                   int end) {
  int i;
  for (i = start; i < end; i++) {
    const Instruction *insc = tape_get(tape, i);
    if (RES != insc->op && PUSH != insc->op) {
      continue;
    }
    if (INSTRUCTION_ID != insc->type || insc->id != NIL_KEYWORD) {
      continue;
    }
    o_Replace(oh, i, _for_op(insc->op == RES ? RNIL : PNIL));
  }
}

void optimizer_ResAidx(OptimizeHelper *oh, const Tape *const tape, int start,
                       int end) {
  int i;
  for (i = start + 1; i < end; i++) {
    const Instruction *first = tape_get(tape, i - 1);
    const Instruction *second = tape_get(tape, i);
    if (RES == first->op &&
        (INSTRUCTION_ID == first->type ||
         INSTRUCTION_PRIMITIVE == first->type) &&
        AIDX == second->op && INSTRUCTION_NO_ARG == second->type &&
        NULL == map_lookup(&oh->i_gotos, as_ptr(i - 1))) {
      o_SetOp(oh, i - 1, AIDX);
      o_Remove(oh, i);
    }
  }
}

void optimizer_StringConcat(OptimizeHelper *oh, const Tape *const tape,
                            int start, int end) {
  // START:
  //   push STR
  //   push STR
  //   add
  // FOLLOWED BY:
  //   push
  //   pust STR
  //   add

  int i;
  for (i = start + 2; i < end; i++) {
    const Instruction *first = tape_get(tape, i - 2);
    const Instruction *second = tape_get(tape, i - 1);
    const Instruction *third = tape_get(tape, i);
    if (PUSH == first->op && INSTRUCTION_STRING == first->type &&
        PUSH == second->op && INSTRUCTION_STRING == second->type &&
        ADD == third->op && INSTRUCTION_NO_ARG == third->type &&
        NULL == map_lookup(&oh->i_gotos, as_ptr(i - 2)) &&
        NULL == map_lookup(&oh->i_gotos, as_ptr(i - 1))) {

      Q strs;
      Q_init(&strs);
      *Q_add_last(&strs) = (char *)first->str;
      *Q_add_last(&strs) = (char *)second->str;

      int j;
      for (j = i + 3; j < end; j += 3) {
        const Instruction *push1 = tape_get(tape, j - 2);
        const Instruction *push2 = tape_get(tape, j - 1);
        const Instruction *add = tape_get(tape, j);
        if (PUSH == push1->op && INSTRUCTION_NO_ARG == push1->type &&
            PUSH == push2->op && INSTRUCTION_STRING == push2->type &&
            ADD == add->op && INSTRUCTION_NO_ARG == add->type &&
            NULL == map_lookup(&oh->i_gotos, as_ptr(j - 2)) &&
            NULL == map_lookup(&oh->i_gotos, as_ptr(j - 1))) {
          *Q_add_last(&strs) = (char *)push2->str;
          o_Remove(oh, j - 2);
          o_Remove(oh, j - 1);
          o_Remove(oh, j);
        } else {
          break;
        }
      }

      int total_len = 0;
      Q_iter iter = Q_iterator(&strs);
      for (; Q_has(&iter); Q_inc(&iter)) {
        char *str = *(char **)Q_value(&iter);
        total_len += strlen(str);
      }
      char *tmp_str = MNEW_ARR(char, total_len + 1);
      tmp_str[0] = '\0';
      iter = Q_iterator(&strs);
      for (; Q_has(&iter); Q_inc(&iter)) {
        char *str = *(char **)Q_value(&iter);
        strcat(tmp_str, str);
      }
      tmp_str[total_len] = '\0';
      char *new_str = intern(tmp_str);
      Instruction new_ins = {
          .op = RES, .type = INSTRUCTION_STRING, .str = new_str};
      o_Replace(oh, i - 2, new_ins);
      o_Remove(oh, i - 1);
      o_Remove(oh, i);
      RELEASE(tmp_str);
      i = j;
    }
  }
}