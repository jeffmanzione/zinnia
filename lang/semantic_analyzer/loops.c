#include "lang/semantic_analyzer/definitions.h"

#include "alloc/arena/intern.h"
#include "debug/debug.h"
#include "lang/lexer/lang_lexer.h"
#include "lang/parser/lang_parser.h"
#include "vm/intern.h"

POPULATE_IMPL(foreach_statement, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  ASSERT(!HAS_TOKEN(stree), CHILD_IS_TOKEN(stree, 0, KEYWORD_FOR));
  foreach_statement->for_token = CHILD_SYNTAX_AT(stree, 0)->token;

  const bool has_parens = CHILD_IS_TOKEN(stree, 1, SYMBOL_LPAREN);

  ASSERT(CHILD_IS_TOKEN(stree, has_parens ? 3 : 2, KEYWORD_IN));
  foreach_statement->in_token =
      CHILD_SYNTAX_AT(stree, has_parens ? 3 : 2)->token;
  foreach_statement->assignment_lhs =
      populate_assignment(analyzer, CHILD_SYNTAX_AT(stree, has_parens ? 2 : 1));
  foreach_statement->iterable = semantic_analyzer_populate(
      analyzer, CHILD_SYNTAX_AT(stree, has_parens ? 4 : 3));
  foreach_statement->body = semantic_analyzer_populate(
      analyzer, CHILD_SYNTAX_AT(stree, has_parens ? 6 : 4));
}

DELETE_IMPL(foreach_statement, SemanticAnalyzer *analyzer) {
  delete_assignment(analyzer, &foreach_statement->assignment_lhs);
  semantic_analyzer_delete(analyzer, foreach_statement->iterable);
  semantic_analyzer_delete(analyzer, foreach_statement->body);
}

PRODUCE_IMPL(foreach_statement, SemanticAnalyzer *analyzer, Tape *target) {
  int num_ins = 0;
  num_ins += num_ins +=
      tape_ins_no_arg(target, NBLK, foreach_statement->for_token);
  num_ins +=
      semantic_analyzer_produce(analyzer, foreach_statement->iterable, target);
  num_ins += tape_ins_no_arg(target, PUSH, foreach_statement->in_token);
  num_ins +=
      tape_ins_text(target, CALL, ITER_FN_NAME, foreach_statement->in_token);
  num_ins += tape_ins_no_arg(target, PUSH, foreach_statement->in_token);

  Tape *tmp = tape_create();
  int body_ins = tape_ins_no_arg(tmp, DUP, foreach_statement->for_token);
  body_ins +=
      tape_ins_text(tmp, CALL, NEXT_FN_NAME, foreach_statement->in_token);
  body_ins += produce_assignment(analyzer, &foreach_statement->assignment_lhs,
                                 tmp, foreach_statement->in_token);
  body_ins += semantic_analyzer_produce(analyzer, foreach_statement->body, tmp);

  int inc_lines = tape_ins_no_arg(target, DUP, foreach_statement->in_token);
  inc_lines += tape_ins_text(target, CALL, HAS_NEXT_FN_NAME,
                             foreach_statement->in_token);
  inc_lines +=
      tape_ins_int(target, IFN, body_ins + 1, foreach_statement->in_token);

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

  tape_append(target, tmp);
  num_ins += body_ins + inc_lines;
  num_ins += tape_ins_int(target, JMP, -(inc_lines + body_ins + 1),
                          foreach_statement->in_token);
  num_ins += tape_ins_no_arg(target, RES, foreach_statement->in_token);
  num_ins += tape_ins_no_arg(target, BBLK, foreach_statement->for_token);
  return num_ins;
}

POPULATE_IMPL(for_statement, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  ASSERT(!HAS_TOKEN(stree), CHILD_IS_TOKEN(stree, 0, KEYWORD_FOR));
  for_statement->for_token = CHILD_SYNTAX_AT(stree, 0)->token;

  const bool has_parens = CHILD_IS_TOKEN(stree, 1, SYMBOL_LPAREN);

  for_statement->init = semantic_analyzer_populate(
      analyzer, CHILD_SYNTAX_AT(stree, has_parens ? 2 : 1));
  for_statement->condition = semantic_analyzer_populate(
      analyzer, CHILD_SYNTAX_AT(stree, has_parens ? 4 : 3));
  for_statement->inc = semantic_analyzer_populate(
      analyzer, CHILD_SYNTAX_AT(stree, has_parens ? 6 : 5));
  for_statement->body = semantic_analyzer_populate(
      analyzer, CHILD_SYNTAX_AT(stree, has_parens ? 8 : 6));
}

DELETE_IMPL(for_statement, SemanticAnalyzer *analyzer) {
  semantic_analyzer_delete(analyzer, for_statement->init);
  semantic_analyzer_delete(analyzer, for_statement->condition);
  semantic_analyzer_delete(analyzer, for_statement->inc);
  semantic_analyzer_delete(analyzer, for_statement->body);
}

PRODUCE_IMPL(for_statement, SemanticAnalyzer *analyzer, Tape *target) {
  int num_ins = 0;
  num_ins += tape_ins_no_arg(target, NBLK, for_statement->for_token);
  num_ins += semantic_analyzer_produce(analyzer, for_statement->init, target);
  int condition_ins =
      semantic_analyzer_produce(analyzer, for_statement->condition, target);
  num_ins += condition_ins;

  Tape *tmp_tape = tape_create();
  int body_ins =
      semantic_analyzer_produce(analyzer, for_statement->body, tmp_tape);
  int inc_ins =
      semantic_analyzer_produce(analyzer, for_statement->inc, tmp_tape);

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
  num_ins += body_ins + inc_ins;
  num_ins += tape_ins_int(target, IFN, body_ins + inc_ins + 1,
                          for_statement->for_token);
  tape_append(target, tmp_tape);

  num_ins +=
      tape_ins_int(target, JMP, -(body_ins + condition_ins + inc_ins + 1 + 1),
                   for_statement->for_token);
  num_ins += tape_ins_no_arg(target, BBLK, for_statement->for_token);

  return num_ins;
}

POPULATE_IMPL(while_statement, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  ASSERT(!HAS_TOKEN(stree), CHILD_IS_TOKEN(stree, 0, KEYWORD_WHILE));
  while_statement->while_token = CHILD_SYNTAX_AT(stree, 0)->token;
  const bool has_parens = CHILD_IS_TOKEN(stree, 1, SYMBOL_LPAREN);
  while_statement->condition = semantic_analyzer_populate(
      analyzer, CHILD_SYNTAX_AT(stree, has_parens ? 2 : 1));
  while_statement->body = semantic_analyzer_populate(
      analyzer, CHILD_SYNTAX_AT(stree, has_parens ? 4 : 2));
}

DELETE_IMPL(while_statement, SemanticAnalyzer *analyzer) {
  semantic_analyzer_delete(analyzer, while_statement->condition);
  semantic_analyzer_delete(analyzer, while_statement->body);
}

PRODUCE_IMPL(while_statement, SemanticAnalyzer *analyzer, Tape *target) {
  int num_ins = 0;
  num_ins += tape_ins_no_arg(target, NBLK, while_statement->while_token);
  num_ins +=
      semantic_analyzer_produce(analyzer, while_statement->condition, target);

  Tape *tmp_tape = tape_create();
  int lines_for_body =
      semantic_analyzer_produce(analyzer, while_statement->body, tmp_tape);
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

  num_ins += lines_for_body + tape_ins_int(target, IFN, lines_for_body + 1,
                                           while_statement->while_token);
  tape_append(target, tmp_tape);

  num_ins += tape_ins_int(target, JMP, -num_ins, while_statement->while_token);
  num_ins += tape_ins_no_arg(target, BBLK, while_statement->while_token);

  return num_ins;
}