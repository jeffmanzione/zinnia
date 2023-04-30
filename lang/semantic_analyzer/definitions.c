#include "lang/semantic_analyzer/definitions.h"

#include "alloc/arena/intern.h"
#include "debug/debug.h"
#include "lang/lexer/lang_lexer.h"
#include "lang/parser/lang_parser.h"
#include "vm/intern.h"

REGISTRATION_FN(semantic_analyzer_init_fn) {
  REGISTER_EXPRESSION_WITH_PRODUCER(identifier);
  REGISTER_EXPRESSION_WITH_PRODUCER(constant);
  REGISTER_EXPRESSION_WITH_PRODUCER(string_literal);
  REGISTER_EXPRESSION_WITH_PRODUCER(tuple_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(array_declaration);
  REGISTER_EXPRESSION_WITH_PRODUCER(primary_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(primary_expression_no_constants);
  REGISTER_EXPRESSION_WITH_PRODUCER(postfix_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(range_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(unary_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(multiplicative_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(additive_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(relational_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(equality_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(and_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(or_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(binary_and_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(binary_xor_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(binary_or_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(in_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(is_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(conditional_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(anon_function_definition);
  REGISTER_EXPRESSION_WITH_PRODUCER(map_declaration);
  REGISTER_EXPRESSION_WITH_PRODUCER(assignment_expression);
  REGISTER_EXPRESSION_WITH_PRODUCER(compound_statement);
  REGISTER_EXPRESSION_WITH_PRODUCER(try_statement);
  REGISTER_EXPRESSION_WITH_PRODUCER(raise_statement);
  REGISTER_EXPRESSION_WITH_PRODUCER(selection_statement);
  REGISTER_EXPRESSION_WITH_PRODUCER(jump_statement);
  REGISTER_EXPRESSION_WITH_PRODUCER(break_statement);
  REGISTER_EXPRESSION_WITH_PRODUCER(exit_statement);
  REGISTER_EXPRESSION_WITH_PRODUCER(foreach_statement);
  REGISTER_EXPRESSION_WITH_PRODUCER(for_statement);
  REGISTER_EXPRESSION_WITH_PRODUCER(while_statement);
  REGISTER_EXPRESSION_WITH_PRODUCER(file_level_statement_list);
  REGISTER_EXPRESSION_WITH_PRODUCER(named_argument);
  REGISTER_EXPRESSION_WITH_PRODUCER(named_argument_list);
}

IMPL_SEMANTIC_ANALYZER_PRODUCE_FN(Tape);

POPULATE_IMPL(identifier, const SyntaxTree *stree, SemanticAnalyzer *analyzer) {
  identifier->id = stree->token;
}

DELETE_IMPL(identifier, SemanticAnalyzer *analyzer) {}

PRODUCE_IMPL(identifier, SemanticAnalyzer *analyzer, Tape *target) {
  return (identifier->id->text == TRUE_KEYWORD)
             ? tape_ins_no_arg(target, RTRU, identifier->id)
         : (identifier->id->text == FALSE_KEYWORD)
             ? tape_ins_no_arg(target, RFLS, identifier->id)
         : (identifier->id->text == NIL_KEYWORD)
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
    num_ins += semantic_analyzer_produce(analyzer, elt, tape);
    num_ins += tape_ins_no_arg(tape, PUSH, tuple_expression->token);
  }
  return num_ins;
}

PRODUCE_IMPL(tuple_expression, SemanticAnalyzer *analyzer, Tape *target) {
  int num_ins = tuple_expression_helper(analyzer, tuple_expression, target);
  num_ins += tape_ins_int(target, TUPL, alist_len(tuple_expression->list),
                          tuple_expression->token);
  return num_ins;
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
        semantic_analyzer_produce(analyzer, array_declaration->exp, target);
    num_ins += tape_ins_no_arg(target, PUSH, array_declaration->token);
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
  FATALF("Unknown primary_expression.");
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
  FATALF("Unknown primary_expression_no_constants.");
}

PRODUCE_IMPL(primary_expression_no_constants, SemanticAnalyzer *analyzer,
             Tape *target) {
  return semantic_analyzer_produce(
      analyzer, primary_expression_no_constants->exp, target);
}

DELETE_IMPL(primary_expression_no_constants, SemanticAnalyzer *analyzer) {
  semantic_analyzer_delete(analyzer, primary_expression_no_constants->exp);
}

void _populate_named_argument(SemanticAnalyzer *analyzer,
                              const SyntaxTree *stree, NamedArgument *arg) {

  arg->id = CHILD_SYNTAX_AT(stree, 0)->token;
  arg->colon = CHILD_SYNTAX_AT(stree, 1)->token;
  arg->value = semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 2));
}

POPULATE_IMPL(named_argument_list, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  named_argument_list->list = alist_create(NamedArgument, DEFAULT_ARRAY_SZ);
  _populate_named_argument(analyzer, CHILD_SYNTAX_AT(stree, 0),
                           alist_add(named_argument_list->list));
  DECLARE_IF_TYPE(tuple1, rule_named_argument_list1, CHILD_SYNTAX_AT(stree, 1));
  // First comma.
  named_argument_list->token = CHILD_SYNTAX_AT(tuple1, 0)->token;
  while (true) {
    ASSERT(CHILD_IS_TOKEN(tuple1, 0, SYMBOL_COMMA));
    _populate_named_argument(analyzer, CHILD_SYNTAX_AT(tuple1, 1),
                             alist_add(named_argument_list->list));
    if (CHILD_COUNT(tuple1) == 2) {
      break;
    }
    ASSERT(CHILD_COUNT(tuple1) == 3);
    ASSIGN_IF_TYPE(tuple1, rule_named_argument_list1,
                   CHILD_SYNTAX_AT(tuple1, 2));
  }
}

PRODUCE_IMPL(named_argument_list, SemanticAnalyzer *analyzer, Tape *target) {
  int num_ins =
      tape_ins_text(target, PUSH, OBJECT_NAME, named_argument_list->token);
  num_ins += tape_ins_no_arg(target, CLLN, named_argument_list->token);
  num_ins += tape_ins_no_arg(target, PUSH, named_argument_list->token);

  AL_iter iter = alist_iter(named_argument_list->list);
  for (; al_has(&iter); al_inc(&iter)) {
    NamedArgument *arg = (NamedArgument *)al_value(&iter);
    num_ins += semantic_analyzer_produce(analyzer, arg->value, target);
    num_ins += tape_ins_no_arg(target, PUSH, arg->colon);
    num_ins += tape_ins_int(target, PEEK, 1, arg->colon);
    num_ins += tape_ins(target, FLD, arg->id);
  }

  num_ins += tape_ins_no_arg(target, RES, named_argument_list->token);
  return num_ins;
}

DELETE_IMPL(named_argument_list, SemanticAnalyzer *analyzer) {
  AL_iter iter = alist_iter(named_argument_list->list);
  for (; al_has(&iter); al_inc(&iter)) {
    NamedArgument *arg = (NamedArgument *)al_value(&iter);
    semantic_analyzer_delete(analyzer, arg->value);
  }
  alist_delete(named_argument_list->list);
}

POPULATE_IMPL(named_argument, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  _populate_named_argument(analyzer, stree, &named_argument->arg);
}

PRODUCE_IMPL(named_argument, SemanticAnalyzer *analyzer, Tape *target) {
  int num_ins =
      tape_ins_text(target, PUSH, OBJECT_NAME, named_argument->arg.colon);
  num_ins += tape_ins_no_arg(target, CLLN, named_argument->arg.colon);
  num_ins += tape_ins_no_arg(target, PUSH, named_argument->arg.colon);
  num_ins +=
      semantic_analyzer_produce(analyzer, named_argument->arg.value, target);
  num_ins += tape_ins_no_arg(target, PUSH, named_argument->arg.colon);
  num_ins += tape_ins_int(target, PEEK, 1, named_argument->arg.colon);
  num_ins += tape_ins(target, FLD, named_argument->arg.id);
  num_ins += tape_ins_no_arg(target, RES, named_argument->arg.colon);
  return num_ins;
}

DELETE_IMPL(named_argument, SemanticAnalyzer *analyzer) {
  semantic_analyzer_delete(analyzer, named_argument->arg.value);
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
                             PostfixType postfix_type, LexType opener,
                             LexType closer) {
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
    FATALF("Unknown postfix.");
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
    num_ins += tape_ins_no_arg(tape, PUSH, cur->token);
    num_ins += semantic_analyzer_produce(analyzer, cur->exp, tape);
    num_ins += tape_ins_no_arg(tape, AIDX, cur->token);
  } else if (cur->type == Postfix_field) {
    // FunctionDef calls on fields must be handled with CALL X.
    if (NULL != *next && (*next)->type == Postfix_fncall) {
      num_ins += tape_ins_no_arg(tape, PUSH, cur->token);
      num_ins += ((*next)->exp != NULL
                      ? semantic_analyzer_produce(analyzer, (*next)->exp, tape)
                      : 0);
      num_ins += tape_ins(tape, (NULL == (*next)->exp) ? CLLN : CALL, cur->id);
      // Advance past the function call since we have already handled it.
      ++(*i);
      *next = (*i + 1 == num_postfix) ? NULL
                                      : (Postfix *)alist_get(suffixes, *i + 1);
    } else {
      num_ins += tape_ins(tape, GET, cur->id);
    }
  } else {
    FATALF("Unknown postfix_expression.");
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
        semantic_analyzer_produce(analyzer, range_expression->inc, target);
    num_ins += tape_ins_no_arg(target, PUSH, range_expression->token);
  }
  num_ins += semantic_analyzer_produce(analyzer, range_expression->end, target);
  num_ins += tape_ins_no_arg(target, PUSH, range_expression->token);
  num_ins +=
      semantic_analyzer_produce(analyzer, range_expression->start, target);
  num_ins += tape_ins_no_arg(target, PUSH, range_expression->token);
  num_ins += tape_ins_int(target, TUPL, range_expression->num_args,
                          range_expression->token);
  num_ins += tape_ins_no_arg(target, CALL, range_expression->token);
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
    FATALF("Unknown unary: %s", token->text);
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
    num_ins += tape_ins_no_arg(target, PUSH, unary_expression->token);
    num_ins += tape_ins_int(target, PUSH, -1, unary_expression->token);
    num_ins += tape_ins_no_arg(target, MULT, unary_expression->token);
    break;
  // TODO: Uncomment when const is implemented.
  // case Unary_const:
  //   num_ins += tape_ins_no_arg(tape, CNST, unary_expression->token);
  //   break;
  case Unary_await:
    num_ins += tape_ins_no_arg(target, WAIT, unary_expression->token);
    break;
  default:
    FATALF("Unknown unary: %s", unary_expression->token);
  }
  return num_ins;
}

BiType relational_type_for_token(const Token *token) {
  switch (token->type) {
  case SYMBOL_STAR:
    return Mult_mult;
  case SYMBOL_FSLASH:
    return Mult_div;
  case SYMBOL_PERCENT:
    return Mult_mod;
  case SYMBOL_PLUS:
    return Add_add;
  case SYMBOL_MINUS:
    return Add_sub;
  case SYMBOL_LTHAN:
    return Rel_lt;
  case SYMBOL_GTHAN:
    return Rel_gt;
  case SYMBOL_LTHANEQ:
    return Rel_lte;
  case SYMBOL_GTHANEQ:
    return Rel_gte;
  case SYMBOL_EQUIV:
    return Rel_eq;
  case SYMBOL_NEQUIV:
    return Rel_neq;
  case KEYWORD_AND:
    return And_and;
  case KEYWORD_OR:
    return And_or;
  case SYMBOL_AMPER:
    return Bin_and;
  case SYMBOL_CARET:
    return Bin_xor;
  case SYMBOL_PIPE:
    return Bin_or;
  default:
    FATALF("Unknown type: %s", token->text);
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
  case And_or:
    return OR;
  case Bin_and:
    return BAND;
  case Bin_xor:
    return BXOR;
  case Bin_or:
    return BOR;
  default:
    FATALF("Unknown type: %s", type);
  }
  return NOP;
}

#define POPULATE_BI_EXPRESSION_IMPL(expr, stree, analyzer)                     \
  {                                                                            \
    expr->exp =                                                                \
        semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 0));       \
    AList *suffixes = alist_create(BiSuffix, DEFAULT_ARRAY_SZ);                \
    SyntaxTree *cur_suffix = CHILD_SYNTAX_AT(stree, 1);                        \
    while (true) {                                                             \
      EXPECT_TYPE(cur_suffix, rule_##expr##1);                                 \
      BiSuffix suffix = {.token = CHILD_SYNTAX_AT(cur_suffix, 0)->token,       \
                         .type = relational_type_for_token(                    \
                             CHILD_SYNTAX_AT(cur_suffix, 0)->token)};          \
      SyntaxTree *second_exp = CHILD_SYNTAX_AT(cur_suffix, 1);                 \
      if (second_exp->rule_fn == stree->rule_fn) {                             \
        suffix.exp = semantic_analyzer_populate(                               \
            analyzer, CHILD_SYNTAX_AT(second_exp, 0));                         \
        alist_append(suffixes, &suffix);                                       \
        cur_suffix = CHILD_SYNTAX_AT(second_exp, 1);                           \
      } else {                                                                 \
        suffix.exp = semantic_analyzer_populate(analyzer, second_exp);         \
        alist_append(suffixes, &suffix);                                       \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
    expr->suffixes = suffixes;                                                 \
  }

#define DELETE_BI_EXPRESSION_IMPL(expr, analyzer)                              \
  {                                                                            \
    semantic_analyzer_delete(analyzer, expr->exp);                             \
    AL_iter iter = alist_iter(expr->suffixes);                                 \
    for (; al_has(&iter); al_inc(&iter)) {                                     \
      BiSuffix *suffix = (BiSuffix *)al_value(&iter);                          \
      semantic_analyzer_delete(analyzer, suffix->exp);                         \
    }                                                                          \
    alist_delete(expr->suffixes);                                              \
  }

#define PRODUCE_BI_EXPRESSION_IMPL(expr, analyzer, tape)                       \
  {                                                                            \
    int num_ins = 0;                                                           \
    num_ins += semantic_analyzer_produce(analyzer, expr->exp, tape);           \
    AL_iter iter = alist_iter(expr->suffixes);                                 \
    for (; al_has(&iter); al_inc(&iter)) {                                     \
      BiSuffix *suffix = (BiSuffix *)al_value(&iter);                          \
      num_ins += tape_ins_no_arg(tape, PUSH, suffix->token);                   \
      num_ins += semantic_analyzer_produce(analyzer, suffix->exp, tape);       \
      num_ins += tape_ins_no_arg(tape, PUSH, suffix->token);                   \
      num_ins +=                                                               \
          tape_ins_no_arg(tape, bi_to_ins(suffix->type), suffix->token);       \
    }                                                                          \
    return num_ins;                                                            \
  }

#define BI_EXPRESSION_IMPL(expr)                                               \
  POPULATE_IMPL(expr, const SyntaxTree *stree, SemanticAnalyzer *analyzer)     \
  POPULATE_BI_EXPRESSION_IMPL(expr, stree, analyzer);                          \
  DELETE_IMPL(expr, SemanticAnalyzer *analyzer)                                \
  DELETE_BI_EXPRESSION_IMPL(expr, analyzer);                                   \
  PRODUCE_IMPL(expr, SemanticAnalyzer *analyzer, Tape *target)                 \
  PRODUCE_BI_EXPRESSION_IMPL(expr, analyzer, target)

#define BI_EXPRESSION_IMPL_NO_PRODUCE(expr)                                    \
  POPULATE_IMPL(expr, const SyntaxTree *stree, SemanticAnalyzer *analyzer)     \
  POPULATE_BI_EXPRESSION_IMPL(expr, stree, analyzer);                          \
  DELETE_IMPL(expr, SemanticAnalyzer *analyzer)                                \
  DELETE_BI_EXPRESSION_IMPL(expr, analyzer);

BI_EXPRESSION_IMPL(multiplicative_expression);
BI_EXPRESSION_IMPL(additive_expression);
BI_EXPRESSION_IMPL(relational_expression);
BI_EXPRESSION_IMPL(equality_expression);
BI_EXPRESSION_IMPL_NO_PRODUCE(and_expression);
BI_EXPRESSION_IMPL_NO_PRODUCE(or_expression);
BI_EXPRESSION_IMPL(binary_and_expression);
BI_EXPRESSION_IMPL(binary_xor_expression);
BI_EXPRESSION_IMPL(binary_or_expression);

// a
// ifn b + 1 + c + 1 + d
// b
// ifn c + 1 + d
// c
// ifn d
// d
PRODUCE_IMPL(and_expression, SemanticAnalyzer *analyzer, Tape *target) {
  AList *and_bodies = alist_create(Tape *, DEFAULT_ARRAY_SZ);
  int num_suffixes = alist_len(and_expression->suffixes);
  int num_ins =
      semantic_analyzer_produce(analyzer, and_expression->exp, target);
  int i, and_suffix_ins = 0;
  for (i = 0; i < num_suffixes; ++i) {
    BiSuffix *suffix = (BiSuffix *)alist_get(and_expression->suffixes, i);
    Tape *and_tape = tape_create();
    and_suffix_ins +=
        semantic_analyzer_produce(analyzer, suffix->exp, and_tape);
    alist_append(and_bodies, &and_tape);
  }

  for (i = 0; i < num_suffixes; ++i) {
    BiSuffix *suffix = (BiSuffix *)alist_get(and_expression->suffixes, i);
    Tape *and_tape = *((Tape **)alist_get(and_bodies, i));
    num_ins += tape_ins_int(target, IFN, and_suffix_ins + num_suffixes - i - 1,
                            suffix->token);
    and_suffix_ins -= tape_size(and_tape);
    num_ins += tape_size(and_tape);
    tape_append(target, and_tape);
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
PRODUCE_IMPL(or_expression, SemanticAnalyzer *analyzer, Tape *target) {
  AList *or_bodies = alist_create(Tape *, DEFAULT_ARRAY_SZ);
  int num_suffixes = alist_len(or_expression->suffixes);
  int num_ins = semantic_analyzer_produce(analyzer, or_expression->exp, target);
  int i, or_suffix_ins = 0;
  for (i = 0; i < num_suffixes; ++i) {
    BiSuffix *suffix = (BiSuffix *)alist_get(or_expression->suffixes, i);
    Tape *or_tape = tape_create();
    or_suffix_ins += semantic_analyzer_produce(analyzer, suffix->exp, or_tape);
    alist_append(or_bodies, &or_tape);
  }

  for (i = 0; i < num_suffixes; ++i) {
    BiSuffix *suffix = (BiSuffix *)alist_get(or_expression->suffixes, i);
    Tape *or_tape = *((Tape **)alist_get(or_bodies, i));
    num_ins += tape_ins_int(target, IF, or_suffix_ins + num_suffixes - i - 1,
                            suffix->token);
    or_suffix_ins -= tape_size(or_tape);
    num_ins += tape_size(or_tape);
    tape_append(target, or_tape);
  }
  alist_delete(or_bodies);
  return num_ins;
}

POPULATE_IMPL(in_expression, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  in_expression->element =
      semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 0));
  in_expression->collection =
      semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 2));
  in_expression->token = CHILD_SYNTAX_AT(stree, 1)->token;
  in_expression->is_not =
      in_expression->token->type == KEYWORD_NOTIN ? true : false;
}

DELETE_IMPL(in_expression, SemanticAnalyzer *analyzer) {
  semantic_analyzer_delete(analyzer, in_expression->element);
  semantic_analyzer_delete(analyzer, in_expression->collection);
}

PRODUCE_IMPL(in_expression, SemanticAnalyzer *analyzer, Tape *target) {
  int num_ins =
      semantic_analyzer_produce(analyzer, in_expression->collection, target);
  num_ins += tape_ins_no_arg(target, PUSH, in_expression->token);
  num_ins +=
      semantic_analyzer_produce(analyzer, in_expression->element, target);
  num_ins += tape_ins_text(target, CALL, IN_FN_NAME, in_expression->token);
  num_ins += (in_expression->is_not
                  ? tape_ins_no_arg(target, NOT, in_expression->token)
                  : 0);
  return num_ins;
}

POPULATE_IMPL(is_expression, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  is_expression->exp =
      semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 0));
  is_expression->type =
      semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 2));
  is_expression->token = CHILD_SYNTAX_AT(stree, 1)->token;
}

DELETE_IMPL(is_expression, SemanticAnalyzer *analyzer) {
  semantic_analyzer_delete(analyzer, is_expression->exp);
  semantic_analyzer_delete(analyzer, is_expression->type);
}

PRODUCE_IMPL(is_expression, SemanticAnalyzer *analyzer, Tape *target) {
  int num_ins = semantic_analyzer_produce(analyzer, is_expression->exp, target);
  num_ins += tape_ins_no_arg(target, PUSH, is_expression->token);
  num_ins += semantic_analyzer_produce(analyzer, is_expression->type, target);
  num_ins += tape_ins_no_arg(target, PUSH, is_expression->token);
  num_ins += tape_ins_no_arg(target, IS, is_expression->token);
  return num_ins;
}

void populate_if_else(SemanticAnalyzer *analyzer, IfElse *if_else,
                      const SyntaxTree *stree) {
  if_else->conditions = alist_create(Conditional, DEFAULT_ARRAY_SZ);
  if_else->else_exp = NULL;
  ASSERT(CHILD_IS_TOKEN(stree, 0, KEYWORD_IF));
  SyntaxTree *if_tree = (SyntaxTree *)stree, *else_body = NULL;
  while (true) {
    Conditional cond = {.condition = semantic_analyzer_populate(
                            analyzer, CHILD_SYNTAX_AT(if_tree, 1)),
                        .if_token = CHILD_SYNTAX_AT(if_tree, 0)->token};
    SyntaxTree *if_body = CHILD_IS_TOKEN(if_tree, 2, KEYWORD_THEN)
                              ? CHILD_SYNTAX_AT(if_tree, 3)
                              : CHILD_SYNTAX_AT(if_tree, 2);
    // Handles else statements.
    if (CHILD_COUNT(if_tree) >= 4) {
      else_body = CHILD_IS_TOKEN(if_tree, 2, KEYWORD_THEN)
                      ? CHILD_SYNTAX_AT(if_tree, 5)
                      : CHILD_SYNTAX_AT(if_tree, 4);
    } else {
      else_body = NULL;
    }
    cond.body = semantic_analyzer_populate(analyzer, if_body);
    alist_append(if_else->conditions, &cond);

    // Is this the final else?
    if (NULL != else_body && !IS_SYNTAX(else_body, stree->rule_fn)) {
      if_else->else_exp = semantic_analyzer_populate(analyzer, else_body);
      break;
    } else if (NULL == else_body) {
      break;
    }
    if_tree = else_body;
  }
}

POPULATE_IMPL(conditional_expression, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  populate_if_else(analyzer, &conditional_expression->if_else, stree);
}

void delete_if_else(SemanticAnalyzer *analyzer, IfElse *if_else) {
  int i;
  for (i = 0; i < alist_len(if_else->conditions); ++i) {
    Conditional *cond = (Conditional *)alist_get(if_else->conditions, i);
    semantic_analyzer_delete(analyzer, cond->condition);
    semantic_analyzer_delete(analyzer, cond->body);
  }
  alist_delete(if_else->conditions);
  if (NULL != if_else->else_exp) {
    semantic_analyzer_delete(analyzer, if_else->else_exp);
  }
}

DELETE_IMPL(conditional_expression, SemanticAnalyzer *analyzer) {
  delete_if_else(analyzer, &conditional_expression->if_else);
}

int produce_if_else(SemanticAnalyzer *analyzer, IfElse *if_else, Tape *tape) {
  int i, num_ins = 0, num_conds = alist_len(if_else->conditions),
         num_cond_ins = 0, num_body_ins = 0;

  AList *conds = alist_create(Tape *, DEFAULT_ARRAY_SZ);
  AList *bodies = alist_create(Tape *, DEFAULT_ARRAY_SZ);
  for (i = 0; i < num_conds; ++i) {
    Conditional *cond = (Conditional *)alist_get(if_else->conditions, i);
    Tape *condition = tape_create();
    Tape *body = tape_create();
    num_cond_ins +=
        semantic_analyzer_produce(analyzer, cond->condition, condition);
    num_body_ins += semantic_analyzer_produce(analyzer, cond->body, body);
    alist_append(conds, &condition);
    alist_append(bodies, &body);
  }

  int num_else_ins = 0;
  Tape *else_body = NULL;
  if (NULL != if_else->else_exp) {
    else_body = tape_create();
    num_else_ins +=
        semantic_analyzer_produce(analyzer, if_else->else_exp, else_body);
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

PRODUCE_IMPL(conditional_expression, SemanticAnalyzer *analyzer, Tape *target) {
  return produce_if_else(analyzer, &conditional_expression->if_else, target);
}

Argument populate_argument(SemanticAnalyzer *analyzer,
                           const SyntaxTree *stree) {
  Argument arg = {.is_const = false,
                  .const_token = NULL,
                  .is_field = false,
                  .has_default = false,
                  .default_value = NULL};
  const SyntaxTree *argument = stree;
  if (IS_SYNTAX(argument, rule_const_function_parameter)) {
    arg.is_const = true;
    arg.const_token = CHILD_SYNTAX_AT(argument, 0)->token;
    argument = CHILD_SYNTAX_AT(argument, 1);
  }
  if (IS_SYNTAX(argument, rule_function_parameter_elt_with_default)) {
    arg.has_default = true;
    arg.default_value = semantic_analyzer_populate(
        analyzer, CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(argument, 1), 1));
    argument = CHILD_SYNTAX_AT(argument, 0);
  }
  ASSERT(IS_SYNTAX(argument, rule_identifier));
  arg.arg_name = argument->token;
  return arg;
}

void add_arg(Arguments *args, Argument *arg) {
  alist_append(args->args, arg);
  if (arg->has_default) {
    args->count_optional++;
  } else {
    args->count_required++;
  }
}

Arguments set_function_args(SemanticAnalyzer *analyzer, const SyntaxTree *stree,
                            const Token *token) {
  Arguments args = {
      .token = token,
      .count_required = 0,
      .count_optional = 0,
      .is_named = false,
  };
  args.args = alist_create(Argument, 4);
  if (IS_SYNTAX(stree, rule_function_named_parameters)) {
    args.is_named = true;
    stree = CHILD_SYNTAX_AT(stree, 1);
  }
  if (!IS_SYNTAX(stree, rule_function_parameter_list)) {
    Argument arg = populate_argument(analyzer, stree);
    add_arg(&args, &arg);
    return args;
  }
  Argument arg = populate_argument(analyzer, CHILD_SYNTAX_AT(stree, 0));
  add_arg(&args, &arg);
  const SyntaxTree *cur = CHILD_SYNTAX_AT(stree, 1);
  while (true) {
    Argument arg = populate_argument(analyzer, CHILD_SYNTAX_AT(cur, 1));
    add_arg(&args, &arg);
    if (CHILD_COUNT(cur) == 2) {
      // Must be last arg.
      break;
    }
    cur = CHILD_SYNTAX_AT(cur, 2);
  }
  return args;
}

void _populate_function_qualifier(const SyntaxTree *fn_qualifier,
                                  bool *is_const, const Token **const_token,
                                  bool *is_async, const Token **async_token) {
  ASSERT(IS_SYNTAX(fn_qualifier, rule_function_qualifier));
  if (IS_TOKEN(fn_qualifier, KEYWORD_CONST)) {
    *is_const = true;
    *const_token = fn_qualifier->token;
  } else if (IS_TOKEN(fn_qualifier, KEYWORD_ASYNC)) {
    *is_async = true;
    *async_token = fn_qualifier->token;
  } else {
    FATALF("Unknown function qualifier.");
  }
}

void populate_function_qualifiers(const SyntaxTree *fn_qualifiers,
                                  bool *is_const, const Token **const_token,
                                  bool *is_async, const Token **async_token) {
  if (IS_SYNTAX(fn_qualifiers, rule_function_qualifier)) {
    _populate_function_qualifier(fn_qualifiers, is_const, const_token, is_async,
                                 async_token);
  } else if (IS_SYNTAX(fn_qualifiers, rule_function_qualifier_list)) {
    ASSERT(CHILD_IS_SYNTAX(fn_qualifiers, 0, rule_function_qualifier));
    ASSERT(CHILD_IS_SYNTAX(fn_qualifiers, 1, rule_function_qualifier));
    _populate_function_qualifier(CHILD_SYNTAX_AT(fn_qualifiers, 0), is_const,
                                 const_token, is_async, async_token);
    _populate_function_qualifier(CHILD_SYNTAX_AT(fn_qualifiers, 1), is_const,
                                 const_token, is_async, async_token);
  } else {
    FATALF("unknown function qualifier list.");
  }
}

void delete_argument(SemanticAnalyzer *analyzer, Argument *arg) {
  if (!arg->has_default) {
    return;
  }
  semantic_analyzer_delete(analyzer, arg->default_value);
}

void _delete_argument_elt(SemanticAnalyzer *analyzer, void *ptr) {
  Argument *arg = (Argument *)ptr;
  delete_argument(analyzer, arg);
}

void delete_arguments(SemanticAnalyzer *analyzer, Arguments *args) {
  AL_iter iter = alist_iter(args->args);
  for (; al_has(&iter); al_inc(&iter)) {
    _delete_argument_elt(analyzer, al_value(&iter));
  }
  alist_delete(args->args);
}

void delete_annotation(SemanticAnalyzer *analyzer, Annotation *annot) {
  if (!annot->has_args || NULL == annot->args_tuple) {
    return;
  }
  semantic_analyzer_delete(analyzer, annot->args_tuple);
}

void delete_function(SemanticAnalyzer *analyzer, FunctionDef *func) {
  if (func->has_args) {
    delete_arguments(analyzer, &func->args);
  }
  if (func->has_annot) {
    delete_annotation(analyzer, &func->annot);
  }
  semantic_analyzer_delete(analyzer, func->body);
}

int produce_argument(Argument *arg, Tape *tape) {
  if (arg->is_field) {
    int num_ins = tape_ins_no_arg(tape, PUSH, arg->arg_name);
    num_ins += tape_ins_text(tape, RES, SELF, arg->arg_name);
    num_ins += tape_ins(tape, arg->is_const ? FLDC : FLD, arg->arg_name);
    return num_ins;
  }
  return tape_ins(tape, arg->is_const ? LETC : LET, arg->arg_name);
}

int produce_all_arguments(SemanticAnalyzer *analyzer, Arguments *args,
                          Tape *tape) {
  int i, num_ins = 0, num_args = alist_len(args->args);
  for (i = 0; i < num_args; ++i) {
    Argument *arg = (Argument *)alist_get(args->args, i);
    if (arg->has_default) {
      if (args->is_named) {
        num_ins += tape_ins_no_arg(tape, PEEK, args->token);
        num_ins += tape_ins(tape, GET, arg->arg_name);

        Tape *tmp = tape_create();
        int default_ins =
            semantic_analyzer_produce(analyzer, arg->default_value, tmp);

        num_ins +=
            tape_ins_int(tape, IF, default_ins, arg->arg_name) + default_ins;
        tape_append(tape, tmp);
      } else {
        num_ins += tape_ins_no_arg(tape, PEEK, args->token);
        num_ins += tape_ins_int(tape, TGTE, i + 1, arg->arg_name);

        Tape *tmp = tape_create();
        int default_ins =
            semantic_analyzer_produce(analyzer, arg->default_value, tmp);
        num_ins += tape_ins_int(tape, IFN, 3, arg->arg_name);
        num_ins += tape_ins_no_arg(tape, (i == num_args - 1) ? RES : PEEK,
                                   arg->arg_name);
        num_ins += tape_ins_int(tape, TGET, i, args->token);
        num_ins +=
            tape_ins_int(tape, JMP, default_ins, arg->arg_name) + default_ins;
        tape_append(tape, tmp);
      }
    } else {
      if (i == num_args - 1) {
        // Pop for last arg.
        num_ins += tape_ins_no_arg(tape, RES, args->token);
      } else {
        num_ins += tape_ins_no_arg(tape, PEEK, arg->arg_name);
      }
      if (args->is_named) {
        num_ins += tape_ins(tape, GET, arg->arg_name);
      } else {
        num_ins += tape_ins_int(tape, TGET, i, args->token);
      }
    }
    num_ins += produce_argument(arg, tape);
  }
  return num_ins;
}

int produce_arguments(SemanticAnalyzer *analyzer, Arguments *args, Tape *tape) {
  if (args->is_named) {
    int num_ins = tape_ins_int(tape, IF, 2, args->token);
    num_ins += tape_ins_text(tape, PUSH, OBJECT_NAME, args->token);
    num_ins += tape_ins_no_arg(tape, CLLN, args->token);
    num_ins += tape_ins_no_arg(tape, PUSH, args->token);
    return num_ins + produce_all_arguments(analyzer, args, tape);
  }
  int num_args = alist_len(args->args);
  int i, num_ins = 0;
  if (num_args == 1) {
    Argument *arg = (Argument *)alist_get(args->args, 0);
    if (arg->has_default) {
      Tape *defaults = tape_create();
      int num_default_ins =
          semantic_analyzer_produce(analyzer, arg->default_value, defaults);
      num_ins += num_default_ins;
      num_ins += tape_ins_no_arg(tape, PUSH, arg->arg_name);
      num_ins += tape_ins_int(tape, TGTE, 1, arg->arg_name);
      num_ins += tape_ins_int(tape, IF, num_default_ins + 1, arg->arg_name);
      tape_append(tape, defaults);
      num_ins += tape_ins_int(tape, JMP, 1, arg->arg_name);
      num_ins += tape_ins_no_arg(tape, RES, arg->arg_name);
      num_ins += tape_ins_int(tape, TGET, 0, arg->arg_name);
    }
    num_ins += produce_argument(arg, tape);
    return num_ins;
  }
  num_ins += tape_ins_no_arg(tape, PUSH, args->token);

  // Handle case where only 1 arg is passed and the rest are optional.
  Argument *first = (Argument *)alist_get(args->args, 0);
  num_ins += tape_ins_no_arg(tape, TLEN, first->arg_name);
  num_ins += tape_ins_no_arg(tape, PUSH, first->arg_name);
  num_ins += tape_ins_int(tape, PUSH, -1, first->arg_name);
  num_ins += tape_ins_no_arg(tape, EQ, first->arg_name);

  Tape *defaults = tape_create();
  int defaults_ins = 0;
  defaults_ins += tape_ins_no_arg(defaults, RES, first->arg_name);
  defaults_ins += produce_argument(first, defaults);
  for (i = 1; i < num_args; ++i) {
    Argument *arg = (Argument *)alist_get(args->args, i);
    if (arg->has_default) {
      defaults_ins +=
          semantic_analyzer_produce(analyzer, arg->default_value, defaults);
    } else {
      defaults_ins += tape_ins_no_arg(defaults, RNIL, arg->arg_name);
    }
    defaults_ins += produce_argument(arg, defaults);
  }

  Tape *non_defaults = tape_create();
  int nondefaults_ins = 0;
  nondefaults_ins += produce_all_arguments(analyzer, args, non_defaults);

  defaults_ins += tape_ins_int(defaults, JMP, nondefaults_ins, first->arg_name);

  num_ins += tape_ins_int(tape, IFN, defaults_ins, first->arg_name);
  tape_append(tape, defaults);
  tape_append(tape, non_defaults);
  num_ins += defaults_ins + nondefaults_ins;
  return num_ins;
}

int produce_function_name(FunctionDef *func, Tape *tape) {
  switch (func->special_method) {
  case SpecialMethod__NONE:
    return func->is_async ? tape_label_async(tape, func->fn_name)
                          : tape_label(tape, func->fn_name);
  case SpecialMethod__EQUIV:
    return func->is_async ? tape_label_text_async(tape, EQ_FN_NAME)
                          : tape_label_text(tape, EQ_FN_NAME);
  case SpecialMethod__NEQUIV:
    return func->is_async ? tape_label_text_async(tape, NEQ_FN_NAME)
                          : tape_label_text(tape, NEQ_FN_NAME);
  case SpecialMethod__ARRAY_INDEX:
    return func->is_async ? tape_label_text_async(tape, ARRAYLIKE_INDEX_KEY)
                          : tape_label_text(tape, ARRAYLIKE_INDEX_KEY);
  case SpecialMethod__ARRAY_SET:
    return func->is_async ? tape_label_text_async(tape, ARRAYLIKE_SET_KEY)
                          : tape_label_text(tape, ARRAYLIKE_SET_KEY);
  default:
    FATALF("Unknown SpecialMethod.");
  }
  return 0;
}

int produce_function(SemanticAnalyzer *analyzer, FunctionDef *func,
                     Tape *tape) {
  int num_ins = 0;
  num_ins += produce_function_name(func, tape);
  if (func->has_args) {
    num_ins += produce_arguments(analyzer, &func->args, tape);
  }
  num_ins += semantic_analyzer_produce(analyzer, func->body, tape);
  // TODO: Uncomment when const is implemented.
  // if (func->is_const) {
  //   num_ins += tape_ins_no_arg(tape, CNST, func->const_token);
  // }
  num_ins += tape_ins_no_arg(tape, RET, func->def_token);
  return num_ins;
}

FunctionDef populate_anon_function(SemanticAnalyzer *analyzer,
                                   const SyntaxTree *stree) {
  FunctionDef func = {.has_annot = false};
  ASSERT(IS_SYNTAX(stree, rule_anon_function_definition));

  const SyntaxTree *func_arg_tuple;
  if (CHILD_IS_SYNTAX(stree, 0, rule_anon_signature_with_qualifier)) {
    func_arg_tuple = CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(stree, 0), 0);
    populate_function_qualifiers(CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(stree, 0), 1),
                                 &func.is_const, &func.const_token,
                                 &func.is_async, &func.async_token);
    func.def_token = CHILD_SYNTAX_AT(func_arg_tuple, 0)->token;
  } else if (CHILD_IS_SYNTAX(stree, 0, rule_identifier)) {
    func_arg_tuple = CHILD_SYNTAX_AT(stree, 0);
    func.is_const = false;
    func.is_async = false;
    func.const_token = NULL;
    func.async_token = NULL;
    func.def_token = CHILD_SYNTAX_AT(stree, 0)->token;
  } else {
    func_arg_tuple = CHILD_SYNTAX_AT(stree, 0);
    func.is_const = false;
    func.is_async = false;
    func.const_token = NULL;
    func.async_token = NULL;
    func.def_token = CHILD_SYNTAX_AT(func_arg_tuple, 0)->token;
  }
  func.fn_name = NULL;
  func.has_args =
      !IS_SYNTAX(func_arg_tuple, rule_function_parameters_no_parameters);
  if (func.has_args) {
    ASSERT(IS_SYNTAX(func_arg_tuple, rule_function_parameters_present) ||
           IS_SYNTAX(func_arg_tuple, rule_identifier));
    const SyntaxTree *func_args = CHILD_IS_SYNTAX(stree, 0, rule_identifier)
                                      ? func_arg_tuple
                                      : CHILD_SYNTAX_AT(func_arg_tuple, 1);
    func.args =
        set_function_args(analyzer, func_args,
                          CHILD_IS_SYNTAX(stree, 0, rule_identifier)
                              ? func_arg_tuple->token
                              : CHILD_SYNTAX_AT(func_arg_tuple, 0)->token);
  }
  func.body = semantic_analyzer_populate(
      analyzer, CHILD_IS_SYNTAX(stree, 1, rule_anon_function_lambda_rhs)
                    ? CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(stree, 1), 1)
                    : CHILD_SYNTAX_AT(stree, 1));
  return func;
}

POPULATE_IMPL(anon_function_definition, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  anon_function_definition->func = populate_anon_function(analyzer, stree);
}

DELETE_IMPL(anon_function_definition, SemanticAnalyzer *analyzer) {
  delete_function(analyzer, &anon_function_definition->func);
}

int produce_anon_function(SemanticAnalyzer *analyzer, FunctionDef *func,
                          Tape *tape) {
  int num_ins = 0, func_ins = 0;
  Tape *tmp = tape_create();
  if (func->has_args) {
    func_ins += produce_arguments(analyzer, &func->args, tmp);
  }
  func_ins += semantic_analyzer_produce(analyzer, func->body, tmp);
  // TODO: Uncomment when const is implemented.
  // if (func->is_const) {
  //   func_ins += tape_ins_no_arg(tmp, CNST, func->const_token);
  // }
  func_ins += tape_ins_no_arg(tmp, RET, func->def_token);
  num_ins += tape_ins_int(tape, JMP, func_ins, func->def_token);
  if (func->is_async) {
    num_ins += tape_anon_label_async(tape, func->def_token);
  } else {
    num_ins += tape_anon_label(tape, func->def_token);
  }
  tape_append(tape, tmp);
  num_ins += func_ins + tape_ins_anon(tape, RES, func->def_token);

  return num_ins;
}

PRODUCE_IMPL(anon_function_definition, SemanticAnalyzer *analyzer,
             Tape *target) {
  return produce_anon_function(analyzer, &anon_function_definition->func,
                               target);
}

MapDecEntry populate_map_dec_entry(SemanticAnalyzer *analyzer,
                                   const SyntaxTree *tree) {
  ASSERT(IS_SYNTAX(tree, rule_map_declaration_entry));
  MapDecEntry entry = {
      .colon = CHILD_SYNTAX_AT(tree, 1)->token,
      .lhs = semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(tree, 0)),
      .rhs = semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(tree, 2))};
  return entry;
}

POPULATE_IMPL(map_declaration, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  if (!CHILD_IS_TOKEN(stree, 0, SYMBOL_LBRACE)) {
    FATALF("Map declaration must start with '{'.");
  }
  map_declaration->lbrce = CHILD_SYNTAX_AT(stree, 0)->token;
  // No entries.
  if (CHILD_IS_TOKEN(stree, 1, SYMBOL_RBRACE)) {
    map_declaration->rbrce = CHILD_SYNTAX_AT(stree, 1)->token;
    map_declaration->is_empty = true;
    map_declaration->entries = NULL;
    return;
  }
  const SyntaxTree *body = CHILD_SYNTAX_AT(stree, 1);
  ASSERT(CHILD_IS_TOKEN(stree, 2, SYMBOL_RBRACE));
  map_declaration->rbrce = CHILD_SYNTAX_AT(stree, 2)->token;
  map_declaration->is_empty = false;
  map_declaration->entries = alist_create(MapDecEntry, 4);
  // Only 1 entry.
  if (IS_SYNTAX(body, rule_map_declaration_entry)) {
    MapDecEntry entry = populate_map_dec_entry(analyzer, body);
    alist_append(map_declaration->entries, &entry);
    return;
  }
  // Multiple entries.
  ASSERT(IS_SYNTAX(body, rule_map_declaration_list));
  MapDecEntry first =
      populate_map_dec_entry(analyzer, CHILD_SYNTAX_AT(body, 0));
  alist_append(map_declaration->entries, &first);
  SyntaxTree *remaining = CHILD_SYNTAX_AT(body, 1);
  while (true) {
    if (IS_SYNTAX(remaining, rule_map_declaration_entry)) {
      MapDecEntry entry = populate_map_dec_entry(analyzer, remaining);
      alist_append(map_declaration->entries, &entry);
      break;
    }
    ASSERT(IS_SYNTAX(remaining, rule_map_declaration_entry1));
    // Last entry.
    if (CHILD_COUNT(remaining) == 2) {
      remaining = CHILD_SYNTAX_AT(remaining, 1);
      continue;
    }
    ASSERT(CHILD_IS_SYNTAX(remaining, 1, rule_map_declaration_entry));
    MapDecEntry entry =
        populate_map_dec_entry(analyzer, CHILD_SYNTAX_AT(remaining, 1));
    alist_append(map_declaration->entries, &entry);
    remaining = CHILD_SYNTAX_AT(remaining, 2);
  }
}

DELETE_IMPL(map_declaration, SemanticAnalyzer *analyzer) {
  if (map_declaration->is_empty) {
    return;
  }
  int i;
  for (i = 0; i < alist_len(map_declaration->entries); ++i) {
    MapDecEntry *entry = (MapDecEntry *)alist_get(map_declaration->entries, i);
    semantic_analyzer_delete(analyzer, entry->lhs);
    semantic_analyzer_delete(analyzer, entry->rhs);
  }
  alist_delete(map_declaration->entries);
}

PRODUCE_IMPL(map_declaration, SemanticAnalyzer *analyzer, Tape *target) {
  int num_ins = 0, i;
  num_ins +=
      tape_ins_text(target, PUSH, intern("struct"), map_declaration->lbrce);
  num_ins += tape_ins_text(target, CLLN, intern("Map"), map_declaration->lbrce);
  if (map_declaration->is_empty) {
    return num_ins;
  }
  num_ins += tape_ins_no_arg(target, PUSH, map_declaration->lbrce);
  for (i = 0; i < alist_len(map_declaration->entries); ++i) {
    MapDecEntry *entry = (MapDecEntry *)alist_get(map_declaration->entries, i);
    num_ins += tape_ins_no_arg(target, DUP, entry->colon);
    num_ins += semantic_analyzer_produce(analyzer, entry->rhs, target);
    num_ins += tape_ins_no_arg(target, PUSH, entry->colon);
    num_ins += semantic_analyzer_produce(analyzer, entry->lhs, target);
    num_ins += tape_ins_no_arg(target, PUSH, entry->colon);
    num_ins += tape_ins_int(target, TUPL, 2, entry->colon);
    num_ins += tape_ins_text(target, CALL, ARRAYLIKE_SET_KEY, entry->colon);
  }
  num_ins += tape_ins_no_arg(target, RES, map_declaration->lbrce);
  return num_ins;
}

void populate_single_postfixes(SemanticAnalyzer *analyzer,
                               const SyntaxTree *stree, Postfix *postfix) {
  ASSERT(!HAS_TOKEN(stree), CHILD_HAS_TOKEN(stree, 0));
  postfix->token = CHILD_SYNTAX_AT(stree, 0)->token;
  if (IS_SYNTAX(stree, rule_array_index_assignment)) {
    postfix->type = Postfix_array_index;
    // Inside array brackets.
    postfix->exp =
        semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 1));
  } else if (IS_SYNTAX(stree, rule_function_call_args)) {
    postfix->type = Postfix_fncall;
    if (CHILD_IS_TOKEN(stree, 1, SYMBOL_RPAREN)) {
      // Has no args.
      postfix->exp = NULL;
    } else {
      // Inside fncall parents.
      postfix->exp =
          semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 1));
    }
  } else if (IS_SYNTAX(stree, rule_field_set_value)) {
    postfix->type = Postfix_field;
    postfix->id = CHILD_SYNTAX_AT(stree, 1)->token;
  } else {
    FATALF("Unknown field_expression1");
  }
}

void populate_single_complex(SemanticAnalyzer *analyzer,
                             const SyntaxTree *stree,
                             SingleAssignment *single) {
  single->type = SingleAssignment_complex;
  single->suffixes = alist_create(Postfix, DEFAULT_ARRAY_SZ);
  single->prefix_exp =
      semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 0));
  SyntaxTree *cur = CHILD_SYNTAX_AT(stree, 1);
  while (true) {
    Postfix postfix = {
        .type = Postfix_none, .id = NULL, .exp = NULL, .token = NULL};
    if (IS_SYNTAX(cur, rule_field_expression1) ||
        IS_SYNTAX(cur, rule_field_next)) {
      populate_single_postfixes(analyzer, CHILD_SYNTAX_AT(cur, 0), &postfix);
      alist_append(single->suffixes, &postfix);
      cur = CHILD_SYNTAX_AT(cur, 1); // field_next
    } else {
      populate_single_postfixes(analyzer, cur, &postfix);
      alist_append(single->suffixes, &postfix);
      break;
    }
  }
}

SingleAssignment populate_single(SemanticAnalyzer *analyzer,
                                 const SyntaxTree *stree) {
  SingleAssignment single = {.is_const = false, .const_token = NULL};
  if (IS_SYNTAX(stree, rule_identifier)) {
    single.type = SingleAssignment_var;
    single.var = stree->token;
  } else if (IS_SYNTAX(stree, rule_const_assignment_expression)) {
    single.type = SingleAssignment_var;
    single.is_const = true;
    single.const_token = CHILD_SYNTAX_AT(stree, 0)->token;
    single.var = CHILD_SYNTAX_AT(stree, 1)->token;
  } else if (IS_SYNTAX(stree, rule_field_expression)) {
    populate_single_complex(analyzer, stree, &single);
  } else {
    FATALF("Unknown single assignment.");
  }
  return single;
}

bool is_assignment_single(const SyntaxTree *stree) {
  return IS_SYNTAX(stree, rule_identifier) ||
         IS_SYNTAX(stree, rule_const_assignment_expression) ||
         IS_SYNTAX(stree, rule_field_expression);
}

MultiAssignment populate_list(SemanticAnalyzer *analyzer,
                              const SyntaxTree *stree, LexType open,
                              LexType close) {
  MultiAssignment assignment = {.subargs =
                                    alist_create(Assignment, DEFAULT_ARRAY_SZ)};
  ASSERT(CHILD_IS_TOKEN(stree, 0, open), CHILD_IS_TOKEN(stree, 2, close));
  SyntaxTree *list = CHILD_SYNTAX_AT(stree, 1);

  if (HAS_TOKEN(list) ||
      !CHILD_IS_SYNTAX(list, 1, rule_assignment_tuple_list1)) {
    // Single element list.
    Assignment subarg = populate_assignment(analyzer, list);
    alist_append(assignment.subargs, &subarg);
  } else {
    Assignment subarg = populate_assignment(analyzer, CHILD_SYNTAX_AT(list, 0));
    alist_append(assignment.subargs, &subarg);
    SyntaxTree *cur = CHILD_SYNTAX_AT(list, 1);
    while (true) {
      if (CHILD_COUNT(cur) == 2) {
        // Last in tuple.
        subarg = populate_assignment(analyzer, CHILD_SYNTAX_AT(cur, 1));
        alist_append(assignment.subargs, &subarg);
        break;
      } else {
        // Not last.
        subarg = populate_assignment(analyzer, CHILD_SYNTAX_AT(cur, 1));
        alist_append(assignment.subargs, &subarg);
        cur = CHILD_SYNTAX_AT(cur, 2);
      }
    }
  }
  return assignment;
}

Assignment populate_assignment(SemanticAnalyzer *analyzer,
                               const SyntaxTree *stree) {
  Assignment assignment;
  if (is_assignment_single(stree)) {
    assignment.type = Assignment_single;
    assignment.single = populate_single(analyzer, stree);
  } else if (IS_SYNTAX(stree, rule_assignment_tuple)) {
    assignment.type = Assignment_tuple;
    assignment.multi =
        populate_list(analyzer, stree, SYMBOL_LPAREN, SYMBOL_RPAREN);
  } else if (IS_SYNTAX(stree, rule_assignment_array)) {
    assignment.type = Assignment_array;
    assignment.multi =
        populate_list(analyzer, stree, SYMBOL_LBRACE, SYMBOL_RBRACE);
  } else {
    FATALF("Unknown assignment.");
  }
  return assignment;
}

POPULATE_IMPL(assignment_expression, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  ASSERT(CHILD_IS_TOKEN(stree, 1, SYMBOL_EQUALS));
  assignment_expression->eq_token = CHILD_SYNTAX_AT(stree, 1)->token;
  assignment_expression->rhs =
      semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 2));
  assignment_expression->assignment =
      populate_assignment(analyzer, CHILD_SYNTAX_AT(stree, 0));
}

void delete_postfix(SemanticAnalyzer *analyzer, Postfix *postfix) {
  if ((postfix->type == Postfix_fncall ||
       postfix->type == Postfix_array_index) &&
      NULL != postfix->exp) {
    semantic_analyzer_delete(analyzer, postfix->exp);
  }
}

void delete_assignment(SemanticAnalyzer *analyzer, Assignment *assignment) {
  if (assignment->type == Assignment_single) {
    SingleAssignment *single = &assignment->single;
    if (single->type == SingleAssignment_complex) {
      semantic_analyzer_delete(analyzer, single->prefix_exp);
      AL_iter iter = alist_iter(single->suffixes);
      for (; al_has(&iter); al_inc(&iter)) {
        delete_postfix(analyzer, (Postfix *)al_value(&iter));
      }
      alist_delete(single->suffixes);
    } else {
      // Is SingleAssignment_var, so do nothing.
    }
  } else {
    AL_iter iter = alist_iter(assignment->multi.subargs);
    for (; al_has(&iter); al_inc(&iter)) {
      delete_assignment(analyzer, (Assignment *)al_value(&iter));
    }
    alist_delete(assignment->multi.subargs);
  }
}

DELETE_IMPL(assignment_expression, SemanticAnalyzer *analyzer) {
  delete_assignment(analyzer, &assignment_expression->assignment);
  semantic_analyzer_delete(analyzer, assignment_expression->rhs);
}

int produce_assignment_multi(SemanticAnalyzer *analyzer, MultiAssignment *multi,
                             Tape *tape, const Token *eq_token) {
  int i, num_ins = 0, len = alist_len(multi->subargs);

  num_ins += tape_ins_no_arg(tape, PUSH, eq_token);
  for (i = 0; i < len; ++i) {
    Assignment *assign = (Assignment *)alist_get(multi->subargs, i);
    num_ins += tape_ins_no_arg(tape, (i < len - i) ? PEEK : RES, eq_token);
    num_ins += tape_ins_no_arg(tape, PUSH, eq_token);
    num_ins += tape_ins_int(tape, RES, i, eq_token);
    num_ins += tape_ins_no_arg(tape, AIDX, eq_token);
    num_ins += produce_assignment(analyzer, assign, tape, eq_token);
  }
  return num_ins;
}

int produce_assignment(SemanticAnalyzer *analyzer, Assignment *assign,
                       Tape *tape, const Token *eq_token) {
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
      num_ins += tape_ins_no_arg(tape, PUSH, eq_token);
      num_ins += semantic_analyzer_produce(analyzer, single->prefix_exp, tape);
      int i, len = alist_len(single->suffixes);
      Postfix *next = (Postfix *)alist_get(single->suffixes, 0);
      for (i = 0; i < len - 1; ++i) {
        if (NULL == next) {
          break;
        }
        num_ins += produce_postfix(analyzer, &i, len - 1, single->suffixes,
                                   &next, tape);
      }
      // Last one should be the set part.
      Postfix *postfix = (Postfix *)alist_get(single->suffixes, len - 1);
      if (postfix->type == Postfix_field) {
        num_ins += tape_ins(tape, FLD, postfix->id);
      } else if (postfix->type == Postfix_array_index) {
        num_ins += tape_ins_no_arg(tape, PUSH, postfix->token);
        num_ins += semantic_analyzer_produce(analyzer, postfix->exp, tape);
        num_ins += tape_ins_no_arg(tape, ASET, postfix->token);
      } else {
        FATALF("Unknown postfix.");
      }
    }
  } else if (assign->type == Assignment_array ||
             assign->type == Assignment_tuple) {
    num_ins +=
        produce_assignment_multi(analyzer, &assign->multi, tape, eq_token);
  } else {
    FATALF("Unknown multi.");
  }
  return num_ins;
}

PRODUCE_IMPL(assignment_expression, SemanticAnalyzer *analyzer, Tape *target) {
  int num_ins =
      semantic_analyzer_produce(analyzer, assignment_expression->rhs, target);
  num_ins += produce_assignment(analyzer, &assignment_expression->assignment,
                                target, assignment_expression->eq_token);
  return num_ins;
}