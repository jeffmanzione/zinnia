// assignment.c
//
// Created on: Dec 27, 2019
//     Author: Jeff Manzione

#include "lang/semantics/expressions/assignment.h"

#include "alloc/alloc.h"
#include "debug/debug.h"
#include "lang/parser/parser.h"
#include "lang/semantics/expression_macros.h"
#include "lang/semantics/expression_tree.h"
#include "lang/semantics/expressions/postfix.h"
#include "program/tape.h"
#include "struct/alist.h"

Assignment populate_assignment(const SyntaxTree *stree);

void populate_single_postfixes(const SyntaxTree *stree, Postfix *postfix) {
  ASSERT(!IS_LEAF(stree), IS_LEAF(stree->first));
  postfix->token = stree->first->token;
  if (IS_SYNTAX(stree, array_index_assignment)) {
    postfix->type = Postfix_array_index;
    // Inside array brackets.
    postfix->exp = populate_expression(stree->second->first);
  } else if (IS_SYNTAX(stree, function_call_args)) {
    postfix->type = Postfix_fncall;
    if (IS_TOKEN(stree->second, RPAREN)) {
      // Has no args.
      postfix->exp = NULL;
    } else {
      // Inside fncall parents.
      postfix->exp = populate_expression(stree->second->first);
    }
  } else if (IS_SYNTAX(stree, field_set_value)) {
    postfix->type = Postfix_field;
    postfix->id = stree->second->token;
  } else {
    ERROR("Unknown field_expression1");
  }
}

void populate_single_complex(const SyntaxTree *stree,
                             SingleAssignment *single) {
  single->type = SingleAssignment_complex;
  single->suffixes = alist_create(Postfix, DEFAULT_ARRAY_SZ);
  single->prefix_exp = populate_expression(stree->first);
  SyntaxTree *cur = (SyntaxTree *)stree->second;
  while (true) {
    Postfix postfix = {
        .type = Postfix_none, .id = NULL, .exp = NULL, .token = NULL};
    if (IS_SYNTAX(cur, field_expression1) || IS_SYNTAX(cur, field_next)) {
      populate_single_postfixes(cur->first, &postfix);
      alist_append(single->suffixes, &postfix);
      cur = cur->second;  // field_next
    } else {
      populate_single_postfixes(cur, &postfix);
      alist_append(single->suffixes, &postfix);
      break;
    }
  }
}

SingleAssignment populate_single(const SyntaxTree *stree) {
  SingleAssignment single = {.is_const = false, .const_token = NULL};
  if (IS_SYNTAX(stree, identifier)) {
    single.type = SingleAssignment_var;
    single.var = stree->token;
  } else if (IS_SYNTAX(stree, const_assignment_expression)) {
    single.type = SingleAssignment_var;
    single.is_const = true;
    single.const_token = stree->first->token;
    single.var = stree->second->token;
  } else if (IS_SYNTAX(stree, field_expression)) {
    populate_single_complex(stree, &single);
  } else {
    ERROR("Unknown single assignment.");
  }
  return single;
}

bool is_assignment_single(const SyntaxTree *stree) {
  return IS_SYNTAX(stree, identifier) ||
         IS_SYNTAX(stree, const_assignment_expression) ||
         IS_SYNTAX(stree, field_expression);
}

MultiAssignment populate_list(const SyntaxTree *stree, TokenType open,
                              TokenType close) {
  MultiAssignment assignment = {.subargs =
                                    alist_create(Assignment, DEFAULT_ARRAY_SZ)};
  ASSERT(IS_TOKEN(stree->first, open), !IS_LEAF(stree->second),
         IS_TOKEN(stree->second->second, close));
  SyntaxTree *list = (SyntaxTree *)stree->second->first;

  if (IS_LEAF(list) || !IS_SYNTAX(list->second, assignment_tuple_list1)) {
    // Single element list.
    Assignment subarg = populate_assignment(list);
    alist_append(assignment.subargs, &subarg);
  } else {
    Assignment subarg = populate_assignment(list->first);
    alist_append(assignment.subargs, &subarg);
    SyntaxTree *cur = list->second;
    while (true) {
      if (IS_TOKEN(cur->first, COMMA)) {
        // Last in tuple.
        subarg = populate_assignment(cur->second);
        alist_append(assignment.subargs, &subarg);
        break;
      } else {
        // Not last.
        subarg = populate_assignment(cur->first->second);
        alist_append(assignment.subargs, &subarg);
        cur = cur->second;
      }
    }
  }
  return assignment;
}

Assignment populate_assignment(const SyntaxTree *stree) {
  Assignment assignment;
  if (is_assignment_single(stree)) {
    assignment.type = Assignment_single;
    assignment.single = populate_single(stree);
  } else if (IS_SYNTAX(stree, assignment_tuple)) {
    assignment.type = Assignment_tuple;
    assignment.multi = populate_list(stree, LPAREN, RPAREN);
  } else if (IS_SYNTAX(stree, assignment_array)) {
    assignment.type = Assignment_array;
    assignment.multi = populate_list(stree, LBRAC, RBRAC);
  } else {
    ERROR("Unknown assignment.");
  }
  return assignment;
}

ImplPopulate(assignment_expression, const SyntaxTree *stree) {
  ASSERT(IS_TOKEN(stree->second->first, EQUALS));
  assignment_expression->eq_token = stree->second->first->token;
  assignment_expression->rhs = populate_expression(stree->second->second);
  assignment_expression->assignment = populate_assignment(stree->first);
}

void delete_postfix(Postfix *postfix) {
  if ((postfix->type == Postfix_fncall ||
       postfix->type == Postfix_array_index) &&
      NULL != postfix->exp) {
    delete_expression(postfix->exp);
  }
}

void delete_assignment(Assignment *assignment) {
  if (assignment->type == Assignment_single) {
    SingleAssignment *single = &assignment->single;
    if (single->type == SingleAssignment_complex) {
      delete_expression(single->prefix_exp);
      alist_iterate(single->suffixes, (EAction)delete_postfix);
      alist_delete(single->suffixes);
    } else {
      // Is SingleAssignment_var, so do nothing.
    }
  } else {
    alist_iterate(assignment->multi.subargs, (EAction)delete_assignment);
    alist_delete(assignment->multi.subargs);
  }
}

ImplDelete(assignment_expression) {
  delete_assignment(&assignment_expression->assignment);
  delete_expression(assignment_expression->rhs);
}

int produce_assignment(Assignment *assign, Tape *tape, const Token *eq_token);

int produce_assignment_multi(MultiAssignment *multi, Tape *tape,
                             const Token *eq_token) {
  int i, num_ins = 0, len = alist_len(multi->subargs);

  num_ins += tape_ins_no_arg(tape, PUSH, eq_token);
  for (i = 0; i < len; ++i) {
    Assignment *assign = (Assignment *)alist_get(multi->subargs, i);
    num_ins += tape_ins_no_arg(tape, (i < len - i) ? PEEK : RES, eq_token) +
               tape_ins_no_arg(tape, PUSH, eq_token) +
               tape_ins_int(tape, RES, i, eq_token) +
               tape_ins_no_arg(tape, AIDX, eq_token) +
               produce_assignment(assign, tape, eq_token);
  }
  return num_ins;
}

int produce_assignment(Assignment *assign, Tape *tape, const Token *eq_token) {
  int num_ins = 0;
  if (assign->type == Assignment_single) {
    SingleAssignment *single = &assign->single;
    if (single->type == SingleAssignment_var) {
      if (single->is_const) {
        num_ins += tape_ins(tape, SETC, single->var);
      } else {
        num_ins += tape_ins(tape, SET, single->var);
      }
    } else {
      ASSERT(single->type == SingleAssignment_complex,
             single->prefix_exp != NULL);
      num_ins += tape_ins_no_arg(tape, PUSH, eq_token) +
                 produce_instructions(single->prefix_exp, tape);
      int i, len = alist_len(single->suffixes);
      Postfix *next = (Postfix *)alist_get(single->suffixes, 0);
      for (i = 0; i < len - 1; ++i) {
        if (NULL == next) {
          break;
        }
        num_ins += produce_postfix(&i, len - 1, single->suffixes, &next, tape);
      }
      // Last one should be the set part.
      Postfix *postfix = (Postfix *)alist_get(single->suffixes, len - 1);
      if (postfix->type == Postfix_field) {
        num_ins += tape_ins(tape, FLD, postfix->id);
      } else if (postfix->type == Postfix_array_index) {
        num_ins += tape_ins_no_arg(tape, PUSH, postfix->token) +
                   produce_instructions(postfix->exp, tape) +
                   tape_ins_no_arg(tape, ASET, postfix->token);
      } else {
        ERROR("Unknown postfix.");
      }
    }
  } else if (assign->type == Assignment_array ||
             assign->type == Assignment_tuple) {
    num_ins += produce_assignment_multi(&assign->multi, tape, eq_token);
  } else {
    ERROR("Unknown multi.");
  }
  return num_ins;
}

ImplProduce(assignment_expression, Tape *tape) {
  return produce_instructions(assignment_expression->rhs, tape) +
         produce_assignment(&assignment_expression->assignment, tape,
                            assignment_expression->eq_token);
}
