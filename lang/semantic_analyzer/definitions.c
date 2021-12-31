#include "lang/semantic_analyzer/definitions.h"

#include "alloc/arena/intern.h"
#include "debug/debug.h"
#include "lang/lexer/lang_lexer.h"
#include "lang/parser/lang_parser.h"
#include "vm/intern.h"

POPULATE_IMPL(identifier, const SyntaxTree *stree, SemanticAnalyzer *analyzer) {
  identifier->id = stree->token;
}

DELETE_IMPL(identifier, SemanticAnalyzer *analyzer) {}

PRODUCE_IMPL(identifier, SemanticAnalyzer *analyzer, Tape *target) {
  return (identifier->id->text == TRUE_KEYWORD)
             ? tape_ins_int(target, RES, 1, identifier->id)
         : (identifier->id->text == FALSE_KEYWORD ||
            identifier->id->text == NIL_KEYWORD)
             ? tape_ins_no_arg(target, RNIL, identifier->id)
             : tape_ins(target, RES, identifier->id);
}

POPULATE_IMPL(constant, const SyntaxTree *stree, SemanticAnalyzer *analyzer) {
  constant->token = stree->token;
  constant->value = token_to_primitive(stree->token);
}

DELETE_IMPL(constant, SemanticAnalyzer *analyzer) {}

PRODUCE_IMPL(constant, SemanticAnalyzer *analyzer, Tape *target) {
  return tape_ins(target, RES, constant->token);
}

POPULATE_IMPL(string_literal, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  string_literal->token = stree->token;
  string_literal->str = stree->token->text;
}

DELETE_IMPL(string_literal, SemanticAnalyzer *analyzer) {}

PRODUCE_IMPL(string_literal, SemanticAnalyzer *analyzer, Tape *target) {
  return tape_ins(target, RES, string_literal->token);
}

POPULATE_IMPL(tuple_expression, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  tuple_expression->list = alist_create(ExpressionTree *, DEFAULT_ARRAY_SZ);
  APPEND_TREE(analyzer, tuple_expression->list, CHILD_SYNTAX_AT(stree, 0));
  DECLARE_IF_TYPE(tuple1, rule_tuple_expression1, CHILD_SYNTAX_AT(stree, 1));
  // First comma.
  tuple_expression->token = CHILD_SYNTAX_AT(tuple1, 0)->token;
  while (true) {
    ASSERT(CHILD_IS_TOKEN(tuple1, 0, SYMBOL_COMMA));
    APPEND_TREE(analyzer, tuple_expression->list, CHILD_SYNTAX_AT(tuple1, 1));
    if (CHILD_COUNT(tuple1) == 2) {
      break;
    }
    ASSERT(CHILD_COUNT(tuple1) == 3);
    ASSIGN_IF_TYPE(tuple1, rule_tuple_expression1, CHILD_SYNTAX_AT(tuple1, 2));
  }
}

DELETE_IMPL(tuple_expression, SemanticAnalyzer *analyzer) {
  int i;
  for (i = 0; i < alist_len(tuple_expression->list); ++i) {
    semantic_analyzer_delete(
        analyzer, *((ExpressionTree **)alist_get(tuple_expression->list, i)));
  }
  alist_delete(tuple_expression->list);
}

int tuple_expression_helper(SemanticAnalyzer *analyzer,
                            Expression_tuple_expression *tuple_expression,
                            Tape *tape) {
  int i, num_ins = 0, tuple_len = alist_len(tuple_expression->list);
  // Start from end and go backward.
  for (i = tuple_len - 1; i >= 0; --i) {
    ExpressionTree *elt = EXTRACT_TREE(tuple_expression->list, i);
    num_ins += semantic_analyzer_produce(analyzer, elt, tape) +
               tape_ins_no_arg(tape, PUSH, tuple_expression->token);
  }
  return num_ins;
}

PRODUCE_IMPL(tuple_expression, SemanticAnalyzer *analyzer, Tape *target) {
  return tuple_expression_helper(analyzer, tuple_expression, target) +
         tape_ins_int(target, TUPL, alist_len(tuple_expression->list),
                      tuple_expression->token);
}

POPULATE_IMPL(array_declaration, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  ASSERT(CHILD_HAS_TOKEN(stree, 0));
  array_declaration->token = CHILD_SYNTAX_AT(stree, 0)->token;
  if (CHILD_COUNT(stree) == 2) {
    // No args.
    array_declaration->exp = NULL;
    array_declaration->is_empty = true;
    return;
  }
  array_declaration->is_empty = false;
  // Inside braces.
  array_declaration->exp =
      semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 1));
}

DELETE_IMPL(array_declaration, SemanticAnalyzer *analyzer) {
  if (array_declaration->exp == NULL) {
    return;
  }
  semantic_analyzer_delete(analyzer, array_declaration->exp);
}

PRODUCE_IMPL(array_declaration, SemanticAnalyzer *analyzer, Tape *target) {
  if (array_declaration->is_empty) {
    return tape_ins_no_arg(target, ANEW, array_declaration->token);
  }
  int num_ins = 0, num_members = 1;
  if (IS_EXPRESSION(array_declaration->exp, tuple_expression)) {
    Expression_tuple_expression *tuple_expression =
        EXTRACT_EXPRESSION(array_declaration->exp, tuple_expression);
    num_members = alist_len(tuple_expression->list);
    num_ins += tuple_expression_helper(analyzer, tuple_expression, target);
  } else {
    num_ins +=
        semantic_analyzer_produce(analyzer, array_declaration->exp, target) +
        tape_ins_no_arg(target, PUSH, array_declaration->token);
  }
  num_ins += tape_ins_int(target, ANEW, num_members, array_declaration->token);
  return num_ins;
}

POPULATE_IMPL(primary_expression, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  ASSERT(CHILD_COUNT(stree) == 3);
  if (CHILD_IS_TOKEN(stree, 0, SYMBOL_LPAREN)) {
    primary_expression->token = CHILD_SYNTAX_AT(stree, 0)->token;
    // Inside parenthesis.
    primary_expression->exp =
        semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 1));
    return;
  }
  ERROR("Unknown primary_expression.");
}

DELETE_IMPL(primary_expression, SemanticAnalyzer *analyzer) {
  semantic_analyzer_delete(analyzer, primary_expression->exp);
}

PRODUCE_IMPL(primary_expression, SemanticAnalyzer *analyzer, Tape *target) {
  return semantic_analyzer_produce(analyzer, primary_expression->exp, target);
}

POPULATE_IMPL(primary_expression_no_constants, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  ASSERT(CHILD_COUNT(stree) == 3);
  if (CHILD_IS_TOKEN(stree, 0, SYMBOL_LPAREN)) {
    primary_expression_no_constants->token = CHILD_SYNTAX_AT(stree, 0)->token;
    // Inside parenthesis.
    primary_expression_no_constants->exp =
        semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 1));
    return;
  }
  ERROR("Unknown primary_expression_no_constants.");
}

PRODUCE_IMPL(primary_expression_no_constants, SemanticAnalyzer *analyzer,
             Tape *target) {
  return semantic_analyzer_produce(
      analyzer, primary_expression_no_constants->exp, target);
}

DELETE_IMPL(primary_expression_no_constants, SemanticAnalyzer *analyzer) {
  semantic_analyzer_delete(analyzer, primary_expression_no_constants->exp);
}

void postfix_period(const SyntaxTree *suffix, AList *suffixes) {
  Postfix postfix = {.type = Postfix_field,
                     .token = CHILD_SYNTAX_AT(suffix, 0)->token,
                     .id = NULL,
                     .exp = NULL};
  const SyntaxTree *tail = CHILD_SYNTAX_AT(suffix, 1);
  ASSERT(IS_SYNTAX(tail, rule_identifier) || IS_TOKEN(tail, KEYWORD_NEW));
  postfix.id = tail->token;
  alist_append(suffixes, &postfix);
}

void postfix_surround_helper(SemanticAnalyzer *analyzer,
                             const SyntaxTree *suffix, AList *suffixes,
                             PostfixType postfix_type, TokenType opener,
                             TokenType closer) {
  Postfix postfix = {.type = postfix_type,
                     .token = CHILD_SYNTAX_AT(suffix, 0)->token,
                     .id = NULL,
                     .exp = NULL};
  ASSERT(!HAS_TOKEN(suffix), CHILD_IS_TOKEN(suffix, 0, opener));
  if (CHILD_IS_TOKEN(suffix, 1, closer)) {
    // No args.
    postfix.exp = NULL;
    alist_append(suffixes, &postfix);
    return;
  }
  const SyntaxTree *content = CHILD_SYNTAX_AT(suffix, 1);
  postfix.exp = semantic_analyzer_populate(analyzer, content);
  alist_append(suffixes, &postfix);
}

void postfix_helper(SemanticAnalyzer *analyzer, const SyntaxTree *postfix,
                    AList *suffixes) {
  if (IS_SYNTAX(postfix, rule_postfix_expression1)) {
    postfix_helper(analyzer, CHILD_SYNTAX_AT(postfix, 0), suffixes);
    postfix_helper(analyzer, CHILD_SYNTAX_AT(postfix, 1), suffixes);
    return;
  }
  if (IS_SYNTAX(postfix, rule_member_access)) {
    postfix_period(postfix, suffixes);
  } else if (IS_SYNTAX(postfix, rule_function_call) ||
             IS_SYNTAX(postfix, rule_empty_parens)) {
    postfix_surround_helper(analyzer, postfix, suffixes, Postfix_fncall,
                            SYMBOL_LPAREN, SYMBOL_RPAREN);
  } else if (IS_SYNTAX(postfix, rule_array_indexing)) {
    postfix_surround_helper(analyzer, postfix, suffixes, Postfix_array_index,
                            SYMBOL_LBRACKET, SYMBOL_RBRACKET);
  } else {
    ERROR("Unknown postfix.");
  }
  if (CHILD_IS_SYNTAX(postfix, 1, rule_postfix_expression1)) {
    postfix_helper(analyzer, CHILD_SYNTAX_AT(postfix, 1), suffixes);
  }
}

POPULATE_IMPL(postfix_expression, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  ASSERT(CHILD_COUNT(stree) == 2);
  postfix_expression->prefix =
      semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 0));
  postfix_expression->suffixes = alist_create(Postfix, DEFAULT_ARRAY_SZ);
  SyntaxTree *suffix = CHILD_SYNTAX_AT(stree, 1);
  postfix_helper(analyzer, suffix, postfix_expression->suffixes);
}

int produce_postfix(SemanticAnalyzer *analyzer, int *i, int num_postfix,
                    AList *suffixes, Postfix **next, Tape *tape) {
  int num_ins = 0;
  Postfix *cur = *next;
  *next =
      (*i + 1 == num_postfix) ? NULL : (Postfix *)alist_get(suffixes, *i + 1);
  if (cur->type == Postfix_fncall) {
    num_ins += tape_ins_no_arg(tape, PUSH, cur->token);
    if (cur->exp != NULL) {
      num_ins += semantic_analyzer_produce(analyzer, cur->exp, tape);
    }
    num_ins +=
        tape_ins_no_arg(tape, (NULL == cur->exp) ? CLLN : CALL, cur->token);
  } else if (cur->type == Postfix_array_index) {
    num_ins += tape_ins_no_arg(tape, PUSH, cur->token) +
               semantic_analyzer_produce(analyzer, cur->exp, tape) +
               tape_ins_no_arg(tape, AIDX, cur->token);
  } else if (cur->type == Postfix_field) {
    // FunctionDef calls on fields must be handled with CALL X.
    if (NULL != *next && (*next)->type == Postfix_fncall) {
      num_ins += tape_ins_no_arg(tape, PUSH, cur->token) +
                 ((*next)->exp != NULL
                      ? semantic_analyzer_produce(analyzer, (*next)->exp, tape)
                      : 0) +
                 tape_ins(tape, (NULL == (*next)->exp) ? CLLN : CALL, cur->id);
      // Advance past the function call since we have already handled it.
      ++(*i);
      *next = (*i + 1 == num_postfix) ? NULL
                                      : (Postfix *)alist_get(suffixes, *i + 1);
    } else {
      num_ins += tape_ins(tape, GET, cur->id);
    }
  } else {
    ERROR("Unknown postfix_expression.");
  }
  return num_ins;
}

PRODUCE_IMPL(postfix_expression, SemanticAnalyzer *analyzer, Tape *target) {
  int i, num_ins = 0, num_postfix = alist_len(postfix_expression->suffixes);
  num_ins +=
      semantic_analyzer_produce(analyzer, postfix_expression->prefix, target);
  Postfix *next = (Postfix *)alist_get(postfix_expression->suffixes, 0);
  for (i = 0; i < num_postfix; ++i) {
    if (NULL == next) {
      break;
    }
    num_ins += produce_postfix(analyzer, &i, num_postfix,
                               postfix_expression->suffixes, &next, target);
  }
  return num_ins;
}

DELETE_IMPL(postfix_expression, SemanticAnalyzer *analyzer) {
  semantic_analyzer_delete(analyzer, postfix_expression->prefix);
  int i;
  for (i = 0; i < alist_len(postfix_expression->suffixes); ++i) {
    Postfix *postfix = (Postfix *)alist_get(postfix_expression->suffixes, i);
    if (postfix->type != Postfix_field && NULL != postfix->exp) {
      semantic_analyzer_delete(analyzer, postfix->exp);
    }
  }
  alist_delete(postfix_expression->suffixes);
}

POPULATE_IMPL(range_expression, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  range_expression->start =
      semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 0));
  const SyntaxTree *range_suffix = CHILD_SYNTAX_AT(stree, 1);
  range_expression->token = CHILD_SYNTAX_AT(range_suffix, 0)->token;
  SyntaxTree *after_colon = CHILD_SYNTAX_AT(range_suffix, 1);
  if (CHILD_COUNT(stree) == 3) {
    // Has inc.
    range_expression->num_args = 3;
    range_expression->inc = semantic_analyzer_populate(analyzer, after_colon);
    range_expression->end = semantic_analyzer_populate(
        analyzer, CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(stree, 2), 1));
    return;
  }
  range_expression->num_args = 2;
  range_expression->inc = NULL;
  range_expression->end = semantic_analyzer_populate(analyzer, after_colon);
}

DELETE_IMPL(range_expression, SemanticAnalyzer *analyzer) {
  semantic_analyzer_delete(analyzer, range_expression->start);
  semantic_analyzer_delete(analyzer, range_expression->end);
  if (NULL != range_expression->inc) {
    semantic_analyzer_delete(analyzer, range_expression->inc);
  }
}

PRODUCE_IMPL(range_expression, SemanticAnalyzer *analyzer, Tape *target) {
  int num_ins = 0;
  num_ins +=
      tape_ins_text(target, PUSH, intern("range"), range_expression->token);
  if (NULL != range_expression->inc) {
    num_ins +=
        semantic_analyzer_produce(analyzer, range_expression->inc, target) +
        tape_ins_no_arg(target, PUSH, range_expression->token);
  }
  num_ins +=
      semantic_analyzer_produce(analyzer, range_expression->end, target) +
      tape_ins_no_arg(target, PUSH, range_expression->token) +
      semantic_analyzer_produce(analyzer, range_expression->start, target) +
      tape_ins_no_arg(target, PUSH, range_expression->token) +
      tape_ins_int(target, TUPL, range_expression->num_args,
                   range_expression->token) +
      tape_ins_no_arg(target, CALL, range_expression->token);
  return num_ins;
}

UnaryType unary_token_to_type(const Token *token) {
  switch (token->type) {
  case SYMBOL_TILDE:
    return Unary_not;
  case SYMBOL_EXCLAIM:
    return Unary_notc;
  case SYMBOL_MINUS:
    return Unary_negate;
  case KEYWORD_CONST:
    return Unary_const;
  case KEYWORD_AWAIT:
    return Unary_await;
  default:
    ERROR("Unknown unary: %s", token->text);
  }
  return Unary_unknown;
}

POPULATE_IMPL(unary_expression, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  ASSERT(CHILD_HAS_TOKEN(stree, 0));
  unary_expression->token = CHILD_SYNTAX_AT(stree, 0)->token;
  unary_expression->type = unary_token_to_type(unary_expression->token);
  unary_expression->exp =
      semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 1));
}

DELETE_IMPL(unary_expression, SemanticAnalyzer *analyzer) {
  semantic_analyzer_delete(analyzer, unary_expression->exp);
}

PRODUCE_IMPL(unary_expression, SemanticAnalyzer *analyzer, Tape *target) {
  int num_ins = 0;
  if (unary_expression->type == Unary_negate &&
      rule_constant == unary_expression->exp->type) {
    Expression_constant *constant =
        EXTRACT_EXPRESSION(unary_expression->exp, constant);
    return tape_ins_neg(target, RES, constant->token);
  }
  num_ins += semantic_analyzer_produce(analyzer, unary_expression->exp, target);
  switch (unary_expression->type) {
  case Unary_not:
    num_ins += tape_ins_no_arg(target, NOT, unary_expression->token);
    break;
  case Unary_notc:
    num_ins += tape_ins_no_arg(target, NOTC, unary_expression->token);
    break;
  case Unary_negate:
    num_ins += tape_ins_no_arg(target, PUSH, unary_expression->token) +
               tape_ins_int(target, PUSH, -1, unary_expression->token) +
               tape_ins_no_arg(target, MULT, unary_expression->token);
    break;
  // TODO: Uncomment when const is implemented.
  // case Unary_const:
  //   num_ins += tape_ins_no_arg(tape, CNST, unary_expression->token);
  //   break;
  case Unary_await:
    num_ins += tape_ins_no_arg(target, WAIT, unary_expression->token);
    break;
  default:
    ERROR("Unknown unary: %s", unary_expression->token);
  }
  return num_ins;
}