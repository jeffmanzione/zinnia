// expression.c
//
// Created on: Jun 23, 2018
//     Author: Jeff Manzione

#include "lang/semantics/expressions/expression.h"

#include <stddef.h>

#include "alloc/alloc.h"
#include "alloc/arena/intern.h"
#include "lang/parser/parser.h"
#include "lang/semantics/expression_macros.h"
#include "lang/semantics/expression_tree.h"
#include "lang/semantics/expressions/files.h"
#include "lang/semantics/expressions/postfix.h"
#include "program/tape.h"
#include "struct/map.h"
#include "vm/intern.h"

ExpressionTree *__extract_tree(AList *alist_of_tree, int index) {
  ExpressionTree **tree_ptr2 =
      (ExpressionTree **)alist_get(alist_of_tree, index);
  return *tree_ptr2;
}

ImplPopulate(identifier, const SyntaxTree *stree) {
  identifier->id = stree->token;
}

ImplDelete(identifier) {}

ImplProduce(identifier, Tape *tape) {
  return (identifier->id->text == TRUE_KEYWORD)
             ? tape_ins_int(tape, RES, 1, identifier->id)
         : (identifier->id->text == FALSE_KEYWORD ||
            identifier->id->text == NIL_KEYWORD)
             ? tape_ins_no_arg(tape, RNIL, identifier->id)
             : tape_ins(tape, RES, identifier->id);
}

ImplPopulate(constant, const SyntaxTree *stree) {
  constant->token = stree->token;
  constant->value = token_to_primitive(stree->token);
}

ImplDelete(constant) {}

ImplProduce(constant, Tape *tape) {
  return tape_ins(tape, RES, constant->token);
}

ImplPopulate(string_literal, const SyntaxTree *stree) {
  string_literal->token = stree->token;
  string_literal->str = stree->token->text;
}

ImplDelete(string_literal) {}

ImplProduce(string_literal, Tape *tape) {
  return tape_ins(tape, RES, string_literal->token);
}

ImplPopulate(tuple_expression, const SyntaxTree *stree) {
  tuple_expression->token = stree->second->first->token; // First comma.
  tuple_expression->list = alist_create(ExpressionTree *, DEFAULT_ARRAY_SZ);
  APPEND_TREE(tuple_expression->list, stree->first);
  DECLARE_IF_TYPE(tuple1, tuple_expression1, stree->second);
  // Loop through indices > 0.
  while (true) {
    if (IS_LEAF(tuple1->second) ||
        !IS_SYNTAX(tuple1->second->second, tuple_expression1)) {
      // Last element.
      APPEND_TREE(tuple_expression->list, tuple1->second);
      break;
    } else {
      APPEND_TREE(tuple_expression->list, tuple1->second->first);
      ASSIGN_IF_TYPE(tuple1, tuple_expression1, tuple1->second->second);
    }
  }
}

ImplDelete(tuple_expression) {
  int i;
  for (i = 0; i < alist_len(tuple_expression->list); ++i) {
    delete_expression(
        *((ExpressionTree **)alist_get(tuple_expression->list, i)));
  }
  alist_delete(tuple_expression->list);
}

int tuple_expression_helper(Expression_tuple_expression *tuple_expression,
                            Tape *tape) {
  int i, num_ins = 0, tuple_len = alist_len(tuple_expression->list);
  // Start from end and go backward.
  for (i = tuple_len - 1; i >= 0; --i) {
    ExpressionTree *elt = EXTRACT_TREE(tuple_expression->list, i);
    num_ins += produce_instructions(elt, tape) +
               tape_ins_no_arg(tape, PUSH, tuple_expression->token);
  }
  return num_ins;
}

ImplProduce(tuple_expression, Tape *tape) {
  return tuple_expression_helper(tuple_expression, tape) +
         tape_ins_int(tape, TUPL, alist_len(tuple_expression->list),
                      tuple_expression->token);
}

ImplPopulate(array_declaration, const SyntaxTree *stree) {
  array_declaration->token = stree->first->token;
  if (IS_LEAF(stree->second)) {
    // No args.
    array_declaration->exp = NULL;
    array_declaration->is_empty = true;
    return;
  }
  array_declaration->is_empty = false;
  // Inside braces.
  array_declaration->exp = populate_expression(stree->second->first);
}

ImplDelete(array_declaration) {
  if (array_declaration->exp != NULL) {
    delete_expression(array_declaration->exp);
  }
}

ImplProduce(array_declaration, Tape *tape) {
  if (array_declaration->is_empty) {
    return tape_ins_no_arg(tape, ANEW, array_declaration->token);
  }

  int num_ins = 0, num_members = 1;
  if (IS_EXPRESSION(array_declaration->exp, tuple_expression)) {
    num_members = alist_len(array_declaration->exp->tuple_expression.list);
    num_ins += tuple_expression_helper(
        &array_declaration->exp->tuple_expression, tape);
  } else {
    num_ins += produce_instructions(array_declaration->exp, tape) +
               tape_ins_no_arg(tape, PUSH, array_declaration->token);
  }
  num_ins += tape_ins_int(tape, ANEW, num_members, array_declaration->token);
  return num_ins;
}

ImplPopulate(primary_expression, const SyntaxTree *stree) {
  if (IS_TOKEN(stree->first, LPAREN)) {
    primary_expression->token = stree->first->token;
    // Inside parenthesis.
    primary_expression->exp = populate_expression(stree->second->first);
    return;
  }
  ERROR("Unknown primary_expression.");
}

ImplPopulate(primary_expression_no_constants, const SyntaxTree *stree) {
  if (IS_TOKEN(stree->first, LPAREN)) {
    primary_expression_no_constants->token = stree->first->token;
    // Inside parenthesis.
    primary_expression_no_constants->exp =
        populate_expression(stree->second->first);
    return;
  }
  ERROR("Unknown primary_expression_no_constants.");
}

ImplDelete(primary_expression) { delete_expression(primary_expression->exp); }
ImplDelete(primary_expression_no_constants) {
  delete_expression(primary_expression_no_constants->exp);
}

ImplProduce(primary_expression, Tape *tape) {
  return produce_instructions(primary_expression->exp, tape);
}
ImplProduce(primary_expression_no_constants, Tape *tape) {
  return produce_instructions(primary_expression_no_constants->exp, tape);
}

void postfix_helper(const SyntaxTree *suffix, AList *suffixes);

void postfix_period(const SyntaxTree *ext, const SyntaxTree *tail,
                    AList *suffixes) {
  Postfix postfix = {
      .type = Postfix_field, .token = ext->token, .id = NULL, .exp = NULL};
  if (IS_SYNTAX(tail, identifier) || IS_TOKEN(tail, NEW)) {
    postfix.id = tail->token;
    alist_append(suffixes, &postfix);
    return;
  }
  ASSERT(!IS_LEAF(tail));
  postfix.id = tail->first->token;
  alist_append(suffixes, &postfix);
  postfix_helper(tail->second, suffixes);
}

void postfix_increment(const SyntaxTree *inc, AList *suffixes) {
  Postfix postfix = {.type = inc->token->type == INCREMENT ? Postfix_increment
                                                           : Postfix_decrement,
                     .token = inc->token,
                     .id = NULL,
                     .exp = NULL};
  ASSERT(IS_LEAF(inc));
  alist_append(suffixes, &postfix);
}

void postfix_surround_helper(const SyntaxTree *suffix, AList *suffixes,
                             PostfixType postfix_type, TokenType opener,
                             TokenType closer) {
  Postfix postfix = {.type = postfix_type,
                     .token = suffix->first->token,
                     .id = NULL,
                     .exp = NULL};
  ASSERT(!IS_LEAF(suffix), IS_TOKEN(suffix->first, opener));
  if (IS_TOKEN(suffix->second, closer)) {
    // No args.
    postfix.exp = NULL;
    alist_append(suffixes, &postfix);
    return;
  }
  // FunctionDef call w/o args followed by postfix. E.g., a().b
  if (!IS_LEAF(suffix->second) && IS_TOKEN(suffix->second->first, closer)) {
    // No args.
    postfix.exp = NULL;
    alist_append(suffixes, &postfix);
    postfix_helper(suffix->second->second, suffixes);
    return;
  }

  SyntaxTree *fn_args = suffix->second->first;
  postfix.exp = populate_expression(fn_args);
  alist_append(suffixes, &postfix);

  if (IS_TOKEN(suffix->second->second, closer)) {
    // No additional postfix.
    return;
  }

  // Must be function call with args and followed by postfix. E.g., a(b).c
  ASSERT(IS_TOKEN(suffix->second->second->first, closer));
  postfix_helper(suffix->second->second->second, suffixes);
}

void postfix_helper(const SyntaxTree *suffix, AList *suffixes) {
  if (IS_LEAF(suffix)) {
    if (IS_TOKEN(suffix, INCREMENT) || IS_TOKEN(suffix, DECREMENT)) {
      postfix_increment(suffix, suffixes);
      return;
    } else {
      ERROR("Unknown leaf postfix.");
    }
  }
  const SyntaxTree *ext = suffix->first;
  switch (ext->token->type) {
  case PERIOD:
    postfix_period(ext, suffix->second, suffixes);
    break;
  case LPAREN:
    postfix_surround_helper(suffix, suffixes, Postfix_fncall, LPAREN, RPAREN);
    break;
  case LBRAC:
    postfix_surround_helper(suffix, suffixes, Postfix_array_index, LBRAC,
                            RBRAC);
    break;
  case INCREMENT:
  case DECREMENT:
    postfix_increment(ext, suffixes);
    postfix_helper(suffix->second, suffixes);
    break;
  default:
    ERROR("Unknown postfix.");
  }
}

ImplPopulate(postfix_expression, const SyntaxTree *stree) {
  postfix_expression->prefix = populate_expression(stree->first);
  postfix_expression->suffixes = alist_create(Postfix, DEFAULT_ARRAY_SZ);

  SyntaxTree *suffix = stree->second;
  postfix_helper(suffix, postfix_expression->suffixes);
}

ImplDelete(postfix_expression) {
  delete_expression(postfix_expression->prefix);
  int i;
  for (i = 0; i < alist_len(postfix_expression->suffixes); ++i) {
    Postfix *postfix = (Postfix *)alist_get(postfix_expression->suffixes, i);
    if (postfix->type != Postfix_field && NULL != postfix->exp) {
      delete_expression(postfix->exp);
    }
  }
  alist_delete(postfix_expression->suffixes);
}

int produce_postfix(int *i, int num_postfix, AList *suffixes, Postfix **next,
                    Tape *tape) {
  int num_ins = 0;
  Postfix *cur = *next;
  *next =
      (*i + 1 == num_postfix) ? NULL : (Postfix *)alist_get(suffixes, *i + 1);
  if (cur->type == Postfix_fncall) {
    num_ins += tape_ins_no_arg(tape, PUSH, cur->token);
    if (cur->exp != NULL) {
      num_ins += produce_instructions(cur->exp, tape);
    }
    num_ins +=
        tape_ins_no_arg(tape, (NULL == cur->exp) ? CLLN : CALL, cur->token);
  } else if (cur->type == Postfix_array_index) {
    num_ins += tape_ins_no_arg(tape, PUSH, cur->token) +
               produce_instructions(cur->exp, tape) +
               tape_ins_no_arg(tape, AIDX, cur->token);
  } else if (cur->type == Postfix_field) {
    // FunctionDef calls on fields must be handled with CALL X.
    if (NULL != *next && (*next)->type == Postfix_fncall) {
      num_ins +=
          tape_ins_no_arg(tape, PUSH, cur->token) +
          ((*next)->exp != NULL ? produce_instructions((*next)->exp, tape)
                                : 0) +
          tape_ins(tape, (NULL == (*next)->exp) ? CLLN : CALL, cur->id);
      // Advance past the function call since we have already handled it.
      ++(*i);
      *next = (*i + 1 == num_postfix) ? NULL
                                      : (Postfix *)alist_get(suffixes, *i + 1);
    } else {
      num_ins += tape_ins(tape, GET, cur->id);
    }
  } else if (cur->type == Postfix_increment) {
    // TODO
  } else if (cur->type == Postfix_decrement) {
    // TODO
  } else {
    ERROR("Unknown postfix_expression.");
  }
  return num_ins;
}

ImplProduce(postfix_expression, Tape *tape) {
  int i, num_ins = 0, num_postfix = alist_len(postfix_expression->suffixes);
  num_ins += produce_instructions(postfix_expression->prefix, tape);
  Postfix *next = (Postfix *)alist_get(postfix_expression->suffixes, 0);
  for (i = 0; i < num_postfix; ++i) {
    if (NULL == next) {
      break;
    }
    num_ins += produce_postfix(&i, num_postfix, postfix_expression->suffixes,
                               &next, tape);
  }
  return num_ins;
}

ImplPopulate(range_expression, const SyntaxTree *stree) {
  range_expression->start = populate_expression(stree->first);
  ASSERT(IS_TOKEN(stree->second->first, COLON));
  range_expression->token = stree->second->first->token;
  SyntaxTree *after_first_colon = stree->second->second;
  if (!IS_LEAF(after_first_colon) && !IS_LEAF(after_first_colon->second) &&
      IS_TOKEN(after_first_colon->second->first, COLON)) {
    // Has inc.
    range_expression->num_args = 3;
    range_expression->inc = populate_expression(after_first_colon->first);
    range_expression->end =
        populate_expression(after_first_colon->second->second);
    return;
  }
  range_expression->num_args = 2;
  range_expression->inc = NULL;
  range_expression->end = populate_expression(after_first_colon);
}

ImplDelete(range_expression) {
  delete_expression(range_expression->start);
  delete_expression(range_expression->end);
  if (NULL != range_expression->inc) {
    delete_expression(range_expression->inc);
  }
}

ImplProduce(range_expression, Tape *tape) {
  int num_ins = 0;
  num_ins +=
      tape_ins_text(tape, PUSH, intern("range"), range_expression->token);
  if (NULL != range_expression->inc) {
    num_ins += produce_instructions(range_expression->inc, tape) +
               tape_ins_no_arg(tape, PUSH, range_expression->token);
  }
  num_ins += produce_instructions(range_expression->end, tape) +
             tape_ins_no_arg(tape, PUSH, range_expression->token) +
             produce_instructions(range_expression->start, tape) +
             tape_ins_no_arg(tape, PUSH, range_expression->token) +
             tape_ins_int(tape, TUPL, range_expression->num_args,
                          range_expression->token) +
             tape_ins_no_arg(tape, CALL, range_expression->token);
  return num_ins;
}

UnaryType unary_token_to_type(const Token *token) {
  switch (token->type) {
  case TILDE:
    return Unary_not;
  case EXCLAIM:
    return Unary_notc;
  case MINUS:
    return Unary_negate;
  case CONST_T:
    return Unary_const;
  case AWAIT:
    return Unary_await;
  default:
    ERROR("Unknown unary: %s", token->text);
  }
  return Unary_unknown;
}

ImplPopulate(unary_expression, const SyntaxTree *stree) {
  ASSERT(IS_LEAF(stree->first));
  unary_expression->token = stree->first->token;
  unary_expression->type = unary_token_to_type(unary_expression->token);
  unary_expression->exp = populate_expression(stree->second);
}

ImplDelete(unary_expression) { delete_expression(unary_expression->exp); }

ImplProduce(unary_expression, Tape *tape) {
  int num_ins = 0;
  if (unary_expression->type == Unary_negate &&
      constant == unary_expression->exp->type) {
    return tape_ins_neg(tape, RES, unary_expression->exp->constant.token);
  }
  num_ins += produce_instructions(unary_expression->exp, tape);
  switch (unary_expression->type) {
  case Unary_not:
    num_ins += tape_ins_no_arg(tape, NOT, unary_expression->token);
    break;
  case Unary_notc:
    num_ins += tape_ins_no_arg(tape, NOTC, unary_expression->token);
    break;
  case Unary_negate:
    num_ins += tape_ins_no_arg(tape, PUSH, unary_expression->token) +
               tape_ins_int(tape, PUSH, -1, unary_expression->token) +
               tape_ins_no_arg(tape, MULT, unary_expression->token);
    break;
  // TODO: Uncomment when const is implemented.
  // case Unary_const:
  //   num_ins += tape_ins_no_arg(tape, CNST, unary_expression->token);
  //   break;
  case Unary_await:
    num_ins += tape_ins_no_arg(tape, WAIT, unary_expression->token);
    break;
  default:
    ERROR("Unknown unary: %s", unary_expression->token);
  }
  return num_ins;
}

BiType relational_type_for_token(const Token *token) {
  switch (token->type) {
  case STAR:
    return Mult_mult;
  case FSLASH:
    return Mult_div;
  case PERCENT:
    return Mult_mod;
  case PLUS:
    return Add_add;
  case MINUS:
    return Add_sub;
  case LTHAN:
    return Rel_lt;
  case GTHAN:
    return Rel_gt;
  case LTHANEQ:
    return Rel_lte;
  case GTHANEQ:
    return Rel_gte;
  case EQUIV:
    return Rel_eq;
  case NEQUIV:
    return Rel_neq;
  case AND_T:
    return And_and;
  case CARET:
    return And_xor;
  case OR_T:
    return And_or;
  default:
    ERROR("Unknown type: %s", token->text);
  }
  return BiType_unknown;
}

Op bi_to_ins(BiType type) {
  switch (type) {
  case Mult_mult:
    return MULT;
  case Mult_div:
    return DIV;
  case Mult_mod:
    return MOD;
  case Add_add:
    return ADD;
  case Add_sub:
    return SUB;
  case Rel_lt:
    return LT;
  case Rel_gt:
    return GT;
  case Rel_lte:
    return LTE;
  case Rel_gte:
    return GTE;
  case Rel_eq:
    return EQ;
  case Rel_neq:
    return NEQ;
  case And_and:
    return AND;
  case And_xor:
    return XOR;
  case And_or:
    return OR;
  default:
    ERROR("Unknown type: %s", type);
  }
  return NOP;
}

#define BiExpressionPopulate(expr, stree)                                      \
  {                                                                            \
    expr->exp = populate_expression(stree->first);                             \
    AList *suffixes = alist_create(BiSuffix, DEFAULT_ARRAY_SZ);                \
    SyntaxTree *cur_suffix = stree->second;                                    \
    while (true) {                                                             \
      EXPECT_TYPE(cur_suffix, expr##1);                                        \
      BiSuffix suffix = {                                                      \
          .token = cur_suffix->first->token,                                   \
          .type = relational_type_for_token(cur_suffix->first->token)};        \
      SyntaxTree *second_exp = cur_suffix->second;                             \
      if (second_exp->expression == stree->expression) {                       \
        suffix.exp = populate_expression(second_exp->first);                   \
        alist_append(suffixes, &suffix);                                       \
        cur_suffix = second_exp->second;                                       \
      } else {                                                                 \
        suffix.exp = populate_expression(second_exp);                          \
        alist_append(suffixes, &suffix);                                       \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
    expr->suffixes = suffixes;                                                 \
  }

#define BiExpressionDelete(expr)                                               \
  {                                                                            \
    delete_expression(expr->exp);                                              \
    void delete_expression_inner(void *ptr) {                                  \
      BiSuffix *suffix = (BiSuffix *)ptr;                                      \
      delete_expression(suffix->exp);                                          \
    }                                                                          \
    alist_iterate(expr->suffixes, delete_expression_inner);                    \
    alist_delete(expr->suffixes);                                              \
  }

#define BiExpressionProduce(expr, tape)                                        \
  {                                                                            \
    int num_ins = 0;                                                           \
    num_ins += produce_instructions(expr->exp, tape);                          \
    void iterate_mult(void *ptr) {                                             \
      BiSuffix *suffix = (BiSuffix *)ptr;                                      \
      num_ins +=                                                               \
          tape_ins_no_arg(tape, PUSH, suffix->token) +                         \
          produce_instructions(suffix->exp, tape) +                            \
          tape_ins_no_arg(tape, PUSH, suffix->token) +                         \
          tape_ins_no_arg(tape, bi_to_ins(suffix->type), suffix->token);       \
    }                                                                          \
    alist_iterate(expr->suffixes, iterate_mult);                               \
    return num_ins;                                                            \
  }

#define ImplBiExpression(expr)                                                 \
  ImplPopulate(expr, const SyntaxTree *stree)                                  \
      BiExpressionPopulate(expr, stree);                                       \
  ImplDelete(expr) BiExpressionDelete(expr);                                   \
  ImplProduce(expr, Tape *tape) BiExpressionProduce(expr, tape)

#define ImplBiExpressionNoProduce(expr)                                        \
  ImplPopulate(expr, const SyntaxTree *stree)                                  \
      BiExpressionPopulate(expr, stree);                                       \
  ImplDelete(expr) BiExpressionDelete(expr);

ImplBiExpression(multiplicative_expression);
ImplBiExpression(additive_expression);
ImplBiExpression(relational_expression);
ImplBiExpression(equality_expression);
ImplBiExpressionNoProduce(and_expression);
ImplBiExpression(xor_expression);
ImplBiExpressionNoProduce(or_expression);

// a
// ifn b + 1 + c + 1 + d
// b
// ifn c + 1 + d
// c
// ifn d
// d
ImplProduce(and_expression, Tape *tape) {
  AList *and_bodies = alist_create(Tape *, DEFAULT_ARRAY_SZ);
  int num_suffixes = alist_len(and_expression->suffixes);
  int num_ins = produce_instructions(and_expression->exp, tape);
  int i, and_suffix_ins = 0;
  for (i = 0; i < num_suffixes; ++i) {
    BiSuffix *suffix = (BiSuffix *)alist_get(and_expression->suffixes, i);
    Tape *and_tape = tape_create();
    and_suffix_ins += produce_instructions(suffix->exp, and_tape);
    alist_append(and_bodies, &and_tape);
  }

  for (i = 0; i < num_suffixes; ++i) {
    BiSuffix *suffix = (BiSuffix *)alist_get(and_expression->suffixes, i);
    Tape *and_tape = *((Tape **)alist_get(and_bodies, i));
    num_ins += tape_ins_int(tape, IFN, and_suffix_ins + num_suffixes - i - 1,
                            suffix->token);
    and_suffix_ins -= tape_size(and_tape);
    num_ins += tape_size(and_tape);
    tape_append(tape, and_tape);
  }
  alist_delete(and_bodies);
  return num_ins;
}

// a
// if b + 1 + c + 1 + d
// b
// if c + 1 + d
// c
// if d
// d
ImplProduce(or_expression, Tape *tape) {
  AList *or_bodies = alist_create(Tape *, DEFAULT_ARRAY_SZ);
  int num_suffixes = alist_len(or_expression->suffixes);
  int num_ins = produce_instructions(or_expression->exp, tape);
  int i, or_suffix_ins = 0;
  for (i = 0; i < num_suffixes; ++i) {
    BiSuffix *suffix = (BiSuffix *)alist_get(or_expression->suffixes, i);
    Tape *or_tape = tape_create();
    or_suffix_ins += produce_instructions(suffix->exp, or_tape);
    alist_append(or_bodies, &or_tape);
  }

  for (i = 0; i < num_suffixes; ++i) {
    BiSuffix *suffix = (BiSuffix *)alist_get(or_expression->suffixes, i);
    Tape *or_tape = *((Tape **)alist_get(or_bodies, i));
    num_ins += tape_ins_int(tape, IF, or_suffix_ins + num_suffixes - i - 1,
                            suffix->token);
    or_suffix_ins -= tape_size(or_tape);
    num_ins += tape_size(or_tape);
    tape_append(tape, or_tape);
  }
  alist_delete(or_bodies);
  return num_ins;
}

ImplPopulate(in_expression, const SyntaxTree *stree) {
  in_expression->element = populate_expression(stree->first);
  in_expression->collection = populate_expression(stree->second->second);
  in_expression->token = stree->second->first->token;
  in_expression->is_not = in_expression->token->type == NOTIN ? true : false;
}

ImplDelete(in_expression) {
  delete_expression(in_expression->element);
  delete_expression(in_expression->collection);
}

ImplProduce(in_expression, Tape *tape) {
  return produce_instructions(in_expression->collection, tape) +
         tape_ins_no_arg(tape, PUSH, in_expression->token) +
         produce_instructions(in_expression->element, tape) +
         tape_ins_text(tape, CALL, IN_FN_NAME, in_expression->token) +
         (in_expression->is_not
              ? tape_ins_no_arg(tape, NOT, in_expression->token)
              : 0);
}

ImplPopulate(is_expression, const SyntaxTree *stree) {
  is_expression->exp = populate_expression(stree->first);
  is_expression->type = populate_expression(stree->second->second);
  is_expression->token = stree->second->first->token;
}

ImplDelete(is_expression) {
  delete_expression(is_expression->exp);
  delete_expression(is_expression->type);
}

ImplProduce(is_expression, Tape *tape) {
  return produce_instructions(is_expression->exp, tape) +
         tape_ins_no_arg(tape, PUSH, is_expression->token) +
         produce_instructions(is_expression->type, tape) +
         tape_ins_no_arg(tape, PUSH, is_expression->token) +
         tape_ins_no_arg(tape, IS, is_expression->token);
}

void populate_if_else(IfElse *if_else, const SyntaxTree *stree) {
  if_else->conditions = alist_create(Conditional, DEFAULT_ARRAY_SZ);
  if_else->else_exp = NULL;
  ASSERT(stree->first->token->type == IF_T);
  SyntaxTree *if_tree = (SyntaxTree *)stree, *else_body = NULL;
  while (true) {
    Conditional cond = {.condition =
                            populate_expression(if_tree->second->first),
                        .if_token = if_tree->first->token};
    SyntaxTree *if_body = IS_TOKEN(if_tree->second->second->first, THEN)
                              ? if_tree->second->second->second
                              : if_tree->second->second;
    // Handles else statements.
    if (!IS_LEAF(if_body) && !IS_LEAF(if_body->second) &&
        IS_TOKEN(if_body->second->first, ELSE)) {
      else_body = if_body->second->second;
      if_body = if_body->first;
    } else {
      else_body = NULL;
    }
    cond.body = populate_expression(if_body);
    alist_append(if_else->conditions, &cond);

    // Is this the final else?
    if (NULL != else_body && !IS_SYNTAX(else_body, stree->expression)) {
      if_else->else_exp = populate_expression(else_body);
      break;
    } else if (NULL == else_body) {
      break;
    }
    if_tree = else_body;
  }
}

ImplPopulate(conditional_expression, const SyntaxTree *stree) {
  populate_if_else(&conditional_expression->if_else, stree);
}

void delete_if_else(IfElse *if_else) {
  int i;
  for (i = 0; i < alist_len(if_else->conditions); ++i) {
    Conditional *cond = (Conditional *)alist_get(if_else->conditions, i);
    delete_expression(cond->condition);
    delete_expression(cond->body);
  }
  alist_delete(if_else->conditions);
  if (NULL != if_else->else_exp) {
    delete_expression(if_else->else_exp);
  }
}

ImplDelete(conditional_expression) {
  delete_if_else(&conditional_expression->if_else);
}

int produce_if_else(IfElse *if_else, Tape *tape) {
  int i, num_ins = 0, num_conds = alist_len(if_else->conditions),
         num_cond_ins = 0, num_body_ins = 0;

  AList *conds = alist_create(Tape *, DEFAULT_ARRAY_SZ);
  AList *bodies = alist_create(Tape *, DEFAULT_ARRAY_SZ);
  for (i = 0; i < num_conds; ++i) {
    Conditional *cond = (Conditional *)alist_get(if_else->conditions, i);
    Tape *condition = tape_create();
    Tape *body = tape_create();
    num_cond_ins += produce_instructions(cond->condition, condition);
    num_body_ins += produce_instructions(cond->body, body);
    alist_append(conds, &condition);
    alist_append(bodies, &body);
  }

  int num_else_ins = 0;
  Tape *else_body = NULL;
  if (NULL != if_else->else_exp) {
    else_body = tape_create();
    num_else_ins += produce_instructions(if_else->else_exp, else_body);
  } else {
    // Compensate for missing last jmp.
    num_else_ins -= 1;
  }

  num_ins = num_cond_ins + num_body_ins + (2 * num_conds) + num_else_ins;

  int num_body_jump = num_body_ins;
  // Iterate and write all conditions forward.
  for (i = 0; i < alist_len(if_else->conditions); ++i) {
    Conditional *cond = (Conditional *)alist_get(if_else->conditions, i);
    Tape *condition = *((Tape **)alist_get(conds, i));
    Tape *body = *((Tape **)alist_get(bodies, i));

    num_cond_ins -= tape_size(condition);
    num_body_jump -= tape_size(body);

    tape_append(tape, condition);
    if (i == num_conds - 1) {
      tape_ins_int(tape, IFN,
                   num_body_ins + num_conds +
                       (NULL == if_else->else_exp ? -1 : 0),
                   cond->if_token);
    } else {
      tape_ins_int(tape, IF,
                   num_cond_ins + num_body_jump + 2 * (num_conds - i - 1),
                   cond->if_token);
    }
  }
  // Iterate and write all bodies backward.
  for (i = num_conds - 1; i >= 0; --i) {
    Conditional *cond = (Conditional *)alist_get(if_else->conditions, i);
    Tape *body = *((Tape **)alist_get(bodies, i));
    num_body_ins -= tape_size(body);
    tape_append(tape, body);
    if (i > 0 || NULL != if_else->else_exp) {
      tape_ins_int(tape, JMP, num_else_ins + num_body_ins + i, cond->if_token);
    }
  }
  // Add else if there is one.
  if (NULL != else_body) {
    tape_append(tape, else_body);
  }
  alist_delete(conds);
  alist_delete(bodies);
  return num_ins;
}

ImplProduce(conditional_expression, Tape *tape) {
  return produce_if_else(&conditional_expression->if_else, tape);
}

void set_anon_function_def(const SyntaxTree *fn_identifier, FunctionDef *func) {
  func->def_token = fn_identifier->token;
  func->fn_name = NULL;
}

// FunctionDef populate_anon_function(const SyntaxTree *stree) {
//  return populate_function_variant(
//      stree, anon_function_definition, anon_signature_const,
//      anon_signature_nonconst, anon_identifier, function_arguments_no_args,
//      function_arguments_present, set_anon_function_def, set_function_args);
//}

FunctionDef populate_anon_function(const SyntaxTree *stree) {
  FunctionDef func;
  ASSERT(IS_SYNTAX(stree, anon_function_definition));

  const SyntaxTree *func_arg_tuple;
  if (IS_SYNTAX(stree->first, anon_signature_with_qualifier)) {
    func_arg_tuple = stree->first->first;
    populate_function_qualifiers(stree->first->second, &func.is_const,
                                 &func.const_token, &func.is_async,
                                 &func.async_token);
    func.def_token = func_arg_tuple->first->token;
  } else if (IS_SYNTAX(stree->first, identifier)) {
    func_arg_tuple = stree->first;
    func.is_const = false;
    func.is_async = false;
    func.const_token = NULL;
    func.async_token = NULL;
    func.def_token = func_arg_tuple->token;
  } else {
    func_arg_tuple = stree->first;
    func.is_const = false;
    func.is_async = false;
    func.const_token = NULL;
    func.async_token = NULL;
    func.def_token = func_arg_tuple->first->token;
  }
  func.fn_name = NULL;
  func.has_args = !IS_SYNTAX(func_arg_tuple, function_arguments_no_args);
  if (func.has_args) {
    ASSERT(IS_SYNTAX(func_arg_tuple, function_arguments_present) ||
           IS_SYNTAX(func_arg_tuple, identifier));
    const SyntaxTree *func_args = IS_SYNTAX(stree->first, identifier)
                                      ? func_arg_tuple
                                      : func_arg_tuple->second->first;
    func.args =
        set_function_args(func_args, IS_SYNTAX(stree->first, identifier)
                                         ? func_arg_tuple->token
                                         : func_arg_tuple->first->token);
  }
  func.body = populate_expression(
      IS_SYNTAX(stree->second, anon_function_lambda_rhs) ? stree->second->second
                                                         : stree->second);
  return func;
}

ImplPopulate(anon_function_definition, const SyntaxTree *stree) {
  anon_function_definition->func = populate_anon_function(stree);
}

ImplDelete(anon_function_definition) {
  delete_function(&anon_function_definition->func);
}

int produce_anon_function(FunctionDef *func, Tape *tape) {
  int num_ins = 0, func_ins = 0;
  Tape *tmp = tape_create();
  if (func->has_args) {
    func_ins += produce_arguments(&func->args, tmp);
  }
  func_ins += produce_instructions(func->body, tmp);
  // TODO: Uncomment when const is implemented.
  // if (func->is_const) {
  //   func_ins += tape_ins_no_arg(tmp, CNST, func->const_token);
  // }
  func_ins += tape_ins_no_arg(tmp, RET, func->def_token);
  num_ins += tape_ins_int(tape, JMP, func_ins, func->def_token) +
             (func->is_async ? tape_anon_label_async(tape, func->def_token)
                             : tape_anon_label(tape, func->def_token));
  tape_append(tape, tmp);
  num_ins += func_ins + tape_ins_anon(tape, RES, func->def_token);

  return num_ins;
}

ImplProduce(anon_function_definition, Tape *tape) {
  return produce_anon_function(&anon_function_definition->func, tape);
}

MapDecEntry populate_map_dec_entry(const SyntaxTree *tree) {
  ASSERT(IS_SYNTAX(tree, map_declaration_entry));
  MapDecEntry entry = {.colon = tree->second->first->token,
                       .lhs = populate_expression(tree->first),
                       .rhs = populate_expression(tree->second->second)};
  return entry;
}

ImplPopulate(map_declaration, const SyntaxTree *stree) {
  if (!IS_TOKEN(stree->first, LBRCE)) {
    ERROR("Map declaration must start with '{'.");
  }
  map_declaration->lbrce = stree->first->token;
  // No entries.
  if (IS_TOKEN(stree->second, RBRCE)) {
    map_declaration->rbrce = stree->second->token;
    map_declaration->is_empty = true;
    map_declaration->entries = NULL;
    return;
  }
  const SyntaxTree *body = stree->second->first;
  ASSERT(IS_TOKEN(stree->second->second, RBRCE));
  map_declaration->rbrce = stree->second->second->token;
  map_declaration->is_empty = false;
  map_declaration->entries = alist_create(MapDecEntry, 4);
  // Only 1 entry.
  if (IS_SYNTAX(body, map_declaration_entry)) {
    MapDecEntry entry = populate_map_dec_entry(body);
    alist_append(map_declaration->entries, &entry);
    return;
  }
  // Multiple entries.
  ASSERT(IS_SYNTAX(body, map_declaration_list));
  MapDecEntry first = populate_map_dec_entry(body->first);
  alist_append(map_declaration->entries, &first);
  SyntaxTree *remaining = body->second;
  while (true) {
    if (IS_SYNTAX(remaining, map_declaration_entry)) {
      MapDecEntry entry = populate_map_dec_entry(remaining);
      alist_append(map_declaration->entries, &entry);
      break;
    }
    ASSERT(IS_SYNTAX(remaining, map_declaration_entry1));
    // Last entry.
    if (IS_TOKEN(remaining->first, COMMA)) {
      remaining = remaining->second;
      continue;
    }
    ASSERT(IS_SYNTAX(remaining->first->second, map_declaration_entry));
    MapDecEntry entry = populate_map_dec_entry(remaining->first->second);
    alist_append(map_declaration->entries, &entry);
    remaining = remaining->second;
  }
}

ImplDelete(map_declaration) {
  if (map_declaration->is_empty) {
    return;
  }
  int i;
  for (i = 0; i < alist_len(map_declaration->entries); ++i) {
    MapDecEntry *entry = (MapDecEntry *)alist_get(map_declaration->entries, i);
    delete_expression(entry->lhs);
    delete_expression(entry->rhs);
  }
  alist_delete(map_declaration->entries);
}

ImplProduce(map_declaration, Tape *tape) {
  int num_ins = 0, i;
  num_ins +=
      tape_ins_text(tape, PUSH, intern("struct"), map_declaration->lbrce) +
      tape_ins_text(tape, CLLN, intern("Map"), map_declaration->lbrce);
  if (map_declaration->is_empty) {
    return num_ins;
  }
  num_ins += tape_ins_no_arg(tape, PUSH, map_declaration->lbrce);
  for (i = 0; i < alist_len(map_declaration->entries); ++i) {
    MapDecEntry *entry = (MapDecEntry *)alist_get(map_declaration->entries, i);
    num_ins += tape_ins_no_arg(tape, DUP, entry->colon) +
               produce_instructions(entry->rhs, tape) +
               tape_ins_no_arg(tape, PUSH, entry->colon) +
               produce_instructions(entry->lhs, tape) +
               tape_ins_no_arg(tape, PUSH, entry->colon) +
               tape_ins_int(tape, TUPL, 2, entry->colon) +
               tape_ins_text(tape, CALL, ARRAYLIKE_SET_KEY, entry->colon);
  }
  num_ins += tape_ins_no_arg(tape, RES, map_declaration->lbrce);
  return num_ins;
}
