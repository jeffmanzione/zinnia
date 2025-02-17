#include "lang/semantic_analyzer/definitions.h"

#include <limits.h>

#include "alloc/arena/intern.h"
#include "debug/debug.h"
#include "lang/lexer/lang_lexer.h"
#include "lang/parser/lang_parser.h"
#include "vm/intern.h"

POPULATE_IMPL(compound_statement, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  ASSERT(CHILD_IS_TOKEN(stree, 0, SYMBOL_LBRACE));
  compound_statement->expressions =
      alist_create(ExpressionTree *, DEFAULT_ARRAY_SZ);
  if (CHILD_IS_TOKEN(stree, 1, SYMBOL_RBRACE)) {
    return;
  }
  ASSERT(CHILD_IS_TOKEN(stree, 2, SYMBOL_RBRACE));

  if (!CHILD_IS_SYNTAX(stree, 1, rule_statement_list)) {
    ExpressionTree *elt =
        semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 1));
    alist_append(compound_statement->expressions, &elt);
    return;
  }

  ExpressionTree *first = semantic_analyzer_populate(
      analyzer, CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(stree, 1), 0));
  alist_append(compound_statement->expressions, &first);
  SyntaxTree *cur = CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(stree, 1), 1);
  while (true) {
    if (!IS_SYNTAX(cur, rule_statement_list1)) {
      ExpressionTree *elt = semantic_analyzer_populate(analyzer, cur);
      alist_append(compound_statement->expressions, &elt);
      break;
    }
    ExpressionTree *elt =
        semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(cur, 0));
    alist_append(compound_statement->expressions, &elt);
    cur = CHILD_SYNTAX_AT(cur, 1);
  }
}

DELETE_IMPL(compound_statement, SemanticAnalyzer *analyzer) {
  AL_iter iter = alist_iter(compound_statement->expressions);
  for (; al_has(&iter); al_inc(&iter)) {
    ExpressionTree *tree = *((ExpressionTree **)al_value(&iter));
    semantic_analyzer_delete(analyzer, tree);
  }
  alist_delete(compound_statement->expressions);
}

PRODUCE_IMPL(compound_statement, SemanticAnalyzer *analyzer, Tape *target) {
  int num_ins = 0;
  if (alist_len(compound_statement->expressions) == 0) {
    return 0;
  }
  AL_iter iter = alist_iter(compound_statement->expressions);
  for (; al_has(&iter); al_inc(&iter)) {
    ExpressionTree *tree = *((ExpressionTree **)al_value(&iter));
    num_ins += semantic_analyzer_produce(analyzer, tree, target);
  }
  return num_ins;
}

POPULATE_IMPL(try_statement, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  ASSERT(!HAS_TOKEN(stree), CHILD_HAS_TOKEN(stree, 0),
         CHILD_IS_TOKEN(stree, 0, KEYWORD_TRY));
  try_statement->try_token = CHILD_SYNTAX_AT(stree, 0)->token;
  try_statement->try_body =
      semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 1));

  ASSERT(CHILD_IS_TOKEN(stree, 2, KEYWORD_CATCH));
  try_statement->catch_token = CHILD_SYNTAX_AT(stree, 2)->token;
  // May be surrounded in parenthesis.
  const SyntaxTree *catch_assign_exp =
      CHILD_IS_SYNTAX(stree, 3, rule_catch_assign)
          ? CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(stree, 3), 1)
          : CHILD_SYNTAX_AT(stree, 3);
  try_statement->error_assignment_lhs =
      populate_assignment(analyzer, catch_assign_exp);
  try_statement->catch_body =
      semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 4));
}

DELETE_IMPL(try_statement, SemanticAnalyzer *analyzer) {
  semantic_analyzer_delete(analyzer, try_statement->try_body);
  delete_assignment(analyzer, &try_statement->error_assignment_lhs);
  semantic_analyzer_delete(analyzer, try_statement->catch_body);
}

PRODUCE_IMPL(try_statement, SemanticAnalyzer *analyzer, Tape *target) {
  int num_ins = 0;
  Tape *try_body_tape = tape_create();
  int try_ins = semantic_analyzer_produce(analyzer, try_statement->try_body,
                                          try_body_tape);
  Tape *catch_body_tape = tape_create();
  int catch_ins = semantic_analyzer_produce(analyzer, try_statement->catch_body,
                                            catch_body_tape);

  int goto_pos = try_ins + 1;

  num_ins += tape_ins_no_arg(target, NBLK, try_statement->try_token);
  num_ins += tape_ins_int(target, CTCH, goto_pos, try_statement->catch_token);
  num_ins += try_ins;
  tape_append(target, try_body_tape);

  Tape *error_assign_tape = tape_create();
  int num_assign =
      produce_assignment(analyzer, &try_statement->error_assignment_lhs,
                         error_assign_tape, try_statement->catch_token);

  num_ins += tape_ins_int(target, JMP, catch_ins + num_assign,
                          try_statement->catch_token);
  // Expect error to be in resval
  num_ins += num_assign + catch_ins;
  tape_append(target, error_assign_tape);
  tape_append(target, catch_body_tape);
  num_ins += tape_ins_no_arg(target, BBLK, try_statement->try_token);
  return num_ins;
}

POPULATE_IMPL(raise_statement, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  ASSERT(CHILD_IS_TOKEN(stree, 0, KEYWORD_RAISE));
  raise_statement->raise_token = CHILD_SYNTAX_AT(stree, 0)->token;
  raise_statement->exp =
      semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 1));
}

DELETE_IMPL(raise_statement, SemanticAnalyzer *analyzer) {
  semantic_analyzer_delete(analyzer, raise_statement->exp);
}

PRODUCE_IMPL(raise_statement, SemanticAnalyzer *analyzer, Tape *target) {
  return semantic_analyzer_produce(analyzer, raise_statement->exp, target) +
         tape_ins_no_arg(target, RAIS, raise_statement->raise_token);
}

POPULATE_IMPL(selection_statement, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  populate_if_else(analyzer, &selection_statement->if_else, stree);
}

DELETE_IMPL(selection_statement, SemanticAnalyzer *analyzer) {
  delete_if_else(analyzer, &selection_statement->if_else);
}

PRODUCE_IMPL(selection_statement, SemanticAnalyzer *analyzer, Tape *target) {
  return produce_if_else(analyzer, &selection_statement->if_else, target);
}

POPULATE_IMPL(jump_statement, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  if (IS_TOKEN(stree, KEYWORD_RETURN)) {
    jump_statement->return_token = stree->token;
    jump_statement->exp = NULL;
    return;
  }
  ASSERT(CHILD_IS_TOKEN(stree, 0, KEYWORD_RETURN));
  jump_statement->return_token = CHILD_SYNTAX_AT(stree, 0)->token;
  if (CHILD_IS_TOKEN(stree, 1, SYMBOL_LPAREN)) {
    jump_statement->exp =
        semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 2));
  } else {
    jump_statement->exp =
        semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 1));
  }
}

DELETE_IMPL(jump_statement, SemanticAnalyzer *analyzer) {
  if (NULL != jump_statement->exp) {
    semantic_analyzer_delete(analyzer, jump_statement->exp);
  }
}

PRODUCE_IMPL(jump_statement, SemanticAnalyzer *analyzer, Tape *target) {
  int num_ins = 0;
  if (NULL != jump_statement->exp) {
    num_ins += semantic_analyzer_produce(analyzer, jump_statement->exp, target);
  }
  num_ins += tape_ins_no_arg(target, RET, jump_statement->return_token);
  return num_ins;
}

POPULATE_IMPL(break_statement, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  ASSERT(HAS_TOKEN(stree));
  break_statement->token = stree->token;

  if (IS_TOKEN(stree, KEYWORD_BREAK)) {
    break_statement->type = Break_break;
  } else if (IS_TOKEN(stree, KEYWORD_CONTINUE)) {
    break_statement->type = Break_continue;
  } else {
    FATALF("Unknown break_statement: %s", stree->token->text);
  }
}

DELETE_IMPL(break_statement, SemanticAnalyzer *analyzer) {}

PRODUCE_IMPL(break_statement, SemanticAnalyzer *analyzer, Tape *target) {
  if (break_statement->type == Break_break) {
    // Signals a break.
    return tape_ins_int(target, JMP, 0, break_statement->token);
  } else if (break_statement->type == Break_continue) {
    // Signals a continue.
    return tape_ins_int(target, JMP, INT_MAX, break_statement->token);
  } else {
    FATALF("Unknown break_statement.");
  }
  return 0;
}

POPULATE_IMPL(exit_statement, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  if (IS_TOKEN(stree, KEYWORD_EXIT)) {
    exit_statement->token = stree->token;
    exit_statement->value = NULL;
  } else {
    ASSERT(IS_SYNTAX(stree, rule_exit_statement));
    exit_statement->token = CHILD_SYNTAX_AT(stree, 0)->token;
    exit_statement->value =
        semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 1));
  }
}

DELETE_IMPL(exit_statement, SemanticAnalyzer *analyzer) {
  // Nop.
  if (NULL != exit_statement->value) {
    semantic_analyzer_delete(analyzer, exit_statement->value);
  }
}

PRODUCE_IMPL(exit_statement, SemanticAnalyzer *analyzer, Tape *target) {
  int num_ins = 0;
  if (NULL == exit_statement->value) {
    num_ins += tape_ins_int(target, RES, 0, exit_statement->token);
  } else {
    num_ins +=
        semantic_analyzer_produce(analyzer, exit_statement->value, target);
  }
  num_ins += tape_ins_no_arg(target, EXIT, exit_statement->token);
  return num_ins;
}
