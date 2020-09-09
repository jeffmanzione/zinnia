// loops.c
//
// Created on: Dec 28, 2019
//     Author: Jeff

#include "loops.h"

#include <limits.h>

#include "alloc/arena/intern.h"
#include "debug/debug.h"
#include "lang/parser/parser.h"
#include "lang/semantics/expression_macros.h"
#include "lang/semantics/expression_tree.h"
#include "program/tape.h"
#include "vm/intern.h"

ImplPopulate(foreach_statement, const SyntaxTree *stree) {
  ASSERT(!IS_LEAF(stree), !IS_LEAF(stree->first),
         IS_TOKEN(stree->first->first, FOR));
  foreach_statement->for_token = stree->first->first->token;
  const SyntaxTree *for_inner = IS_TOKEN(stree->first->second->first, LPAREN)
                                    ? stree->first->second->second->first
                                    : stree->first->second;
  ASSERT(!IS_LEAF(for_inner->second), IS_TOKEN(for_inner->second->first, IN));
  foreach_statement->in_token = for_inner->second->first->token;
  foreach_statement->assignment_lhs = populate_assignment(for_inner->first);
  foreach_statement->iterable = populate_expression(for_inner->second->second);
  foreach_statement->body = populate_expression(stree->second);
}

ImplDelete(foreach_statement) {
  delete_assignment(&foreach_statement->assignment_lhs);
  delete_expression(foreach_statement->iterable);
  delete_expression(foreach_statement->body);
}

ImplProduce(foreach_statement, Tape *tape) {
  int num_ins = 0;
  num_ins +=
      tape_ins_no_arg(tape, NBLK, foreach_statement->for_token) +
      produce_instructions(foreach_statement->iterable, tape) +
      tape_ins_no_arg(tape, PUSH, foreach_statement->in_token) +
      tape_ins_text(tape, CALL, ITER_FN_NAME, foreach_statement->in_token) +
      tape_ins_no_arg(tape, PUSH, foreach_statement->in_token);

  Tape *tmp = tape_create();
  int body_ins =
      tape_ins_no_arg(tmp, DUP, foreach_statement->for_token) +
      tape_ins_text(tmp, CALL, NEXT_FN_NAME, foreach_statement->in_token) +
      produce_assignment(&foreach_statement->assignment_lhs, tmp,
                         foreach_statement->in_token) +
      produce_instructions(foreach_statement->body, tmp);

  int inc_lines =
      tape_ins_no_arg(tape, DUP, foreach_statement->in_token) +
      tape_ins_text(tape, CALL, HAS_NEXT_FN_NAME, foreach_statement->in_token) +
      tape_ins_int(tape, IFN, body_ins + 1, foreach_statement->in_token);

  int i;
  for (i = 0; i < tape_size(tmp); ++i) {
    Instruction *ins = tape_get_mutable(tmp, i);
    if (ins->op != JMP) {
      continue;
    }
    // break
    if (pint(&ins->val) == 0) {
      pset_int(&ins->val, body_ins - i);
    }
    // continue
    else if (pint(&ins->val) == INT_MAX) {
      pset_int(&ins->val, body_ins - i - 1);
    }
  }

  tape_append(tape, tmp);
  num_ins += body_ins + inc_lines +
             tape_ins_int(tape, JMP, -(inc_lines + body_ins + 1),
                          foreach_statement->in_token) +
             tape_ins_no_arg(tape, RES, foreach_statement->in_token) +
             tape_ins_no_arg(tape, BBLK, foreach_statement->for_token);
  return num_ins;
}

ImplPopulate(for_statement, const SyntaxTree *stree) {
  ASSERT(!IS_LEAF(stree), !IS_LEAF(stree->first),
         IS_TOKEN(stree->first->first, FOR));
  for_statement->for_token = stree->first->first->token;
  const SyntaxTree *for_inner = IS_TOKEN(stree->first->second->first, LPAREN)
                                    ? stree->first->second->second->first
                                    : stree->first->second;
  ASSERT(!IS_LEAF(for_inner));
  for_statement->init = populate_expression(for_inner->first);
  ASSERT(!IS_LEAF(for_inner->second),
         IS_TOKEN(for_inner->second->first, COMMA));
  for_statement->condition =
      populate_expression(for_inner->second->second->first);
  for_statement->inc =
      populate_expression(for_inner->second->second->second->second);
  for_statement->body = populate_expression(stree->second);
}

ImplDelete(for_statement) {
  delete_expression(for_statement->init);
  delete_expression(for_statement->condition);
  delete_expression(for_statement->inc);
  delete_expression(for_statement->body);
}

ImplProduce(for_statement, Tape *tape) {
  int num_ins = 0;
  num_ins += tape_ins_no_arg(tape, NBLK, for_statement->for_token) +
             produce_instructions(for_statement->init, tape);
  int condition_ins = produce_instructions(for_statement->condition, tape);
  num_ins += condition_ins;

  Tape *tmp_tape = tape_create();
  int body_ins = produce_instructions(for_statement->body, tmp_tape);
  int inc_ins = produce_instructions(for_statement->inc, tmp_tape);

  int i;
  for (i = 0; i < tape_size(tmp_tape); ++i) {
    Instruction *ins = tape_get_mutable(tmp_tape, i);
    if (ins->op != JMP) {
      continue;
    }
    // break
    if (pint(&ins->val) == 0) {
      pset_int(&ins->val, body_ins + inc_ins - i);
    }
    // continue
    else if (pint(&ins->val) == INT_MAX) {
      pset_int(&ins->val, body_ins - i - 1);
    }
  }
  num_ins +=
      body_ins + inc_ins +
      tape_ins_int(tape, IFN, body_ins + inc_ins + 1, for_statement->for_token);
  tape_append(tape, tmp_tape);

  num_ins +=
      tape_ins_int(tape, JMP, -(body_ins + condition_ins + inc_ins + 1 + 1),
                   for_statement->for_token) +
      tape_ins_no_arg(tape, BBLK, for_statement->for_token);

  return num_ins;
}

ImplPopulate(while_statement, const SyntaxTree *stree) {
  ASSERT(!IS_LEAF(stree), !IS_LEAF(stree->first),
         IS_TOKEN(stree->first->first, WHILE));
  while_statement->while_token = stree->first->first->token;
  while_statement->condition = populate_expression(stree->first->second);
  while_statement->body = populate_expression(stree->second);
}

ImplDelete(while_statement) {
  delete_expression(while_statement->condition);
  delete_expression(while_statement->body);
}

ImplProduce(while_statement, Tape *tape) {
  int num_ins = 0;
  num_ins += tape_ins_no_arg(tape, NBLK, while_statement->while_token) +
             produce_instructions(while_statement->condition, tape);

  Tape *tmp_tape = tape_create();
  int lines_for_body = produce_instructions(while_statement->body, tmp_tape);
  int i;
  for (i = 0; i < tape_size(tmp_tape); ++i) {
    Instruction *ins = tape_get_mutable(tmp_tape, i);
    if (ins->op != JMP) {
      continue;
    }
    if (pint(&ins->val) == 0) {
      pset_int(&ins->val, lines_for_body - i);
    } else if (pint(&ins->val) == INT_MAX) {
      pset_int(&ins->val, -(i + 1));
    }
  }

  num_ins += lines_for_body + tape_ins_int(tape, IFN, lines_for_body + 1,
                                           while_statement->while_token);
  tape_append(tape, tmp_tape);

  num_ins += tape_ins_int(tape, JMP, -num_ins, while_statement->while_token) +
             tape_ins_no_arg(tape, BBLK, while_statement->while_token);

  return num_ins;
}
