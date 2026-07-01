// instruction.h
//
// Created on: Jun 6, 2020
//     Author: Jeff Manzione

#include "zinnia/program/tape.h"

#include "language-tools/lexer/token.h"
#include "zinnia/alloc/alloc.h"
#include "zinnia/lang/lexer/lang_lexer.h"
#include "zinnia/program/instruction.h"
#include "zinnia/util/error.h"
#include "zinnia/util/string_util.h"
#include "zinnia/util/void_array.h"
#include "zinnia/vm/intern.h"

#define DEFAULT_TAPE_SZ 64

#define CLASS_KEYWORD "class"
#define CLASSEND_KEYWORD "endclass"
#define MODULE_KEYWORD "module"
#define FIELD_KEYWORD "field"
#define SOURCE_KEYWORD "source"
#define BODY_KEYWORD "body"
#define INSTRUCTION_COMMENT_LPAD 16
#define PADDING ""

IMPL_STABLE_MAPLIKE(ClassRefMap, char *, ClassRef);
IMPL_ARRAYLIKE(SourceMappingArray, SourceMapping);
IMPL_ARRAYLIKE(ClassRefPtrArray, ClassRef *);
IMPL_STABLE_MAPLIKE(FunctionRefMap, char *, FunctionRef);
IMPL_STABLE_MAPLIKE(FieldRefMap, char *, FieldRef);

struct Tape_ {
  const char *module_name;
  InstructionArray ins;

  SourceMappingArray source_map;
  CharPtrArray source_lines;
  char *external_source_fn;

  ClassRefMap class_refs;
  FunctionRefMap func_refs;

  ClassRef *current_class;
};

void classref_init_(ClassRef *ref, const char name[]);
void classref_finalize_(ClassRef *ref);

Tape *tape_create() {
  Tape *tape = MNEW(Tape);
  InstructionArray_init(&tape->ins);
  SourceMappingArray_init(&tape->source_map);
  CharPtrArray_init(&tape->source_lines);
  ClassRefMap_init(&tape->class_refs, hash_interned_string,
                   compare_interned_strings);
  FunctionRefMap_init(&tape->func_refs, hash_interned_string,
                      compare_interned_strings);
  tape->current_class = NULL;
  tape->external_source_fn = NULL;
  tape->module_name = NULL;
  return tape;
}

void tape_delete(Tape *tape) {
  ASSERT(tape != NULL);
  InstructionArray_finalize(&tape->ins);
  SourceMappingArray_finalize(&tape->source_map);
  CharPtrArray_finalize(&tape->source_lines);
  ClassRefMapIOIterator class_i;
  ClassRefMap_io_iterator(&class_i, &tape->class_refs);
  for (; ClassRefMap_io_has_next(&class_i); ClassRefMap_io_next(&class_i)) {
    classref_finalize_(ClassRefMap_io_mutable_value(&class_i));
  }
  ClassRefMap_finalize(&tape->class_refs);
  FunctionRefMap_finalize(&tape->func_refs);
  RELEASE(tape);
}

Instruction *tape_add(Tape *tape) {
  SourceMapping *sm = SourceMappingArray_push_back_ref(&tape->source_map);
  // No sourcemap info present.
  sm->col = -1;
  sm->line = -1;
  sm->source_col = -1;
  sm->source_line = -1;
  sm->source_token = NULL;
  sm->token = NULL;
  return InstructionArray_push_back_ref(&tape->ins);
}

SourceMapping *tape_add_source(Tape *tape, Instruction *ins) {
  ASSERT(tape != NULL);
  ASSERT(ins != NULL);
  uintptr_t index = ins - InstructionArray_get_ref_unchecked(&tape->ins, 0);
  ASSERT(index >= 0);
  ASSERT(index < tape_size(tape));
  return SourceMappingArray_mutable_ref_unchecked(&tape->source_map, index);
}

void tape_set_external_source(Tape *const tape, const char file_name[]) {
  ASSERT(tape != NULL);
  // ASSERT(file_name != NULL);
  tape->external_source_fn = (char *)file_name;
}

const char *tape_get_external_source(const Tape *const tape) {
  return tape->external_source_fn;
}

void tape_start_func_at_index(Tape *tape, const char name[], uint32_t index,
                              bool is_async) {
  ASSERT(tape != NULL);
  ASSERT(name != NULL);
  FunctionRef *ref;
  // In a class.
  const bool inserted =
      (NULL != tape->current_class)
          ? FunctionRefMap_insert(&tape->current_class->func_refs, name,
                                  sizeof(char *), &ref)
          : FunctionRefMap_insert(&tape->func_refs, name, sizeof(char *), &ref);

  if (!inserted) {
    FATALF(
        "Attempting to add function %s, but a function by that name already "
        "exists.",
        name);
  }
  ref->name = name;
  ref->index = index;
  // TODO: Implement const functions.
  ref->is_const = false;
  ref->is_async = is_async;
}

void tape_start_func_(Tape *tape, const char name[], bool is_async) {
  tape_start_func_at_index(tape, name, InstructionArray_size(&tape->ins),
                           is_async);
}

ClassRef *tape_start_class_at_index(Tape *tape, const char name[],
                                    uint32_t index) {
  ASSERT(tape != NULL);
  ASSERT(name != NULL);
  ClassRef *ref;
  if (!ClassRefMap_insert(&tape->class_refs, name, sizeof(char *), &ref)) {
    FATALF(
        "Attempting to add class %s, but a function by that name already "
        "exists.",
        name);
  }
  classref_init_(ref, name);
  ref->start_index = index;
  tape->current_class = ref;
  return ref;
}

void tape_end_class_at_index(Tape *tape, uint32_t index) {
  ASSERT(tape != NULL);
  ASSERT(tape->current_class != NULL);
  tape->current_class->end_index = index;
  tape->current_class = NULL;
}

void tape_start_class(Tape *tape, const char name[]) {
  tape_start_class_at_index(tape, name, InstructionArray_size(&tape->ins));
}

void tape_end_class(Tape *tape) {
  tape_end_class_at_index(tape, InstructionArray_size(&tape->ins));
}

void field_ref_init_(FieldRef *fref, const char name[]) { fref->name = name; }

void tape_field(Tape *tape, const char *field) {
  ClassRef *cls = tape->current_class;
  if (NULL == cls) {
    FATALF("Cannot have field outside of a class.");
  }
  FieldRef *ref;
  if (!FieldRefMap_insert(&cls->field_refs, field, sizeof(char *), &ref)) {
    FATALF(
        "Attempting to add field '%s', but a field by that name already "
        "exists.",
        field);
  }
  field_ref_init_(ref, field);
}

const Instruction *tape_get(const Tape *tape, uint32_t index) {
  ASSERT(tape != NULL);
  ASSERT(index >= 0);
  ASSERT(index < tape_size(tape));
  return InstructionArray_get_ref_unchecked(&tape->ins, index);
}

Instruction *tape_get_mutable(Tape *tape, uint32_t index) {
  ASSERT(tape != NULL);
  ASSERT(index >= 0);
  ASSERT(index < tape_size(tape));
  return InstructionArray_mutable_ref_unchecked(&tape->ins, index);
}

const SourceMapping *tape_get_source(const Tape *tape, uint32_t index) {
  ASSERT(tape != NULL);
  ASSERT(index >= 0);
  ASSERT(index < tape_size(tape));
  return SourceMappingArray_get_ref_unchecked(&tape->source_map, index);
}

const char *tape_get_sourceline(const Tape *const tape, int line) {
  if (line < 0 || line >= CharPtrArray_size(&tape->source_lines)) {
    return NULL;
  }
  const char *escaped_line =
      CharPtrArray_get_unchecked(&tape->source_lines, line);
  char *unescaped_line;
  unescape(escaped_line, &unescaped_line);
  const char *to_return = global_intern(unescaped_line);
  RELEASE(unescaped_line);
  return to_return;
}

size_t tape_size(const Tape *tape) {
  ASSERT(tape != NULL);
  return InstructionArray_size(&tape->ins);
}

uint32_t tape_class_count(const Tape *tape) {
  return ClassRefMap_size(&tape->class_refs);
}

ClassRefMapIOIterator tape_classes(const Tape *tape) {
  ClassRefMapIOIterator it;
  ClassRefMap_io_iterator(&it, &tape->class_refs);
  return it;
}

const ClassRef *tape_get_class(const Tape *tape, const char class_name[]) {
  return ClassRefMap_find_ref((ClassRefMap *)&tape->class_refs, class_name,
                              sizeof(char *));
}

uint32_t tape_func_count(const Tape *tape) {
  return FunctionRefMap_size(&tape->func_refs);
}

FunctionRefMapIOIterator tape_functions(const Tape *tape) {
  FunctionRefMapIOIterator it;
  FunctionRefMap_io_iterator(&it, &tape->func_refs);
  return it;
}

const char *tape_module_name(const Tape *tape) { return tape->module_name; }

void classref_init_(ClassRef *ref, const char name[]) {
  ref->name = name;
  FunctionRefMap_init(&ref->func_refs, hash_interned_string,
                      compare_interned_strings);
  FieldRefMap_init(&ref->field_refs, hash_interned_string,
                   compare_interned_strings);
  CharPtrArray_init(&ref->supers);
}

void classref_finalize_(ClassRef *ref) {
  FunctionRefMap_finalize(&ref->func_refs);
  FieldRefMap_finalize(&ref->field_refs);
  CharPtrArray_finalize(&ref->supers);
}

void tape_write(const Tape *tape, FILE *file, bool minimize) {
  ASSERT(tape != NULL);
  ASSERT(file != NULL);
  if (tape->module_name && 0 != strcmp("$", tape->module_name)) {
    fprintf(file, "module %s\n", tape->module_name);
  }
  if (NULL != tape->external_source_fn) {
    fprintf(file, "source '%s'\n", tape->external_source_fn);
  }
  ClassRefMapIOIterator cls_iter;
  ClassRefMap_io_iterator(&cls_iter, &tape->class_refs);
  FunctionRefMapIOIterator func_iter;
  FunctionRefMap_io_iterator(&func_iter, &tape->func_refs);
  FunctionRefMapIOIterator cls_func_iter;
  bool in_class = false;
  int i;
  for (i = 0; i <= InstructionArray_size(&tape->ins); ++i) {
    if (!in_class && ClassRefMap_io_has_next(&cls_iter)) {
      const ClassRef *class_ref = ClassRefMap_io_value(&cls_iter);
      if (class_ref->start_index == i) {
        in_class = true;
        FunctionRefMap_io_iterator(&cls_func_iter, &class_ref->func_refs);
        fprintf(file, "class %s\n", class_ref->name);

        FieldRefMapIOIterator fields;
        FieldRefMap_io_iterator(&fields, &class_ref->field_refs);
        for (; FieldRefMap_io_has_next(&fields); FieldRefMap_io_next(&fields)) {
          const FieldRef *field = FieldRefMap_io_mutable_value(&fields);
          fprintf(file, "%sfield %s\n", minimize ? "" : " ", field->name);
        }
      }
    }
    if (in_class && FunctionRefMap_io_has_next(&cls_func_iter)) {
      const FunctionRef *func_ref = FunctionRefMap_io_value(&cls_func_iter);
      if (func_ref->index == i) {
        if (func_ref->is_async) {
          fprintf(file, "@%s:async\n", func_ref->name);
        } else {
          fprintf(file, "@%s\n", func_ref->name);
        }
        FunctionRefMap_io_next(&cls_func_iter);
      }
    } else if (FunctionRefMap_io_has_next(&func_iter)) {
      const FunctionRef *func_ref = FunctionRefMap_io_value(&func_iter);
      if (func_ref->index == i) {
        if (func_ref->is_async) {
          fprintf(file, "@%s:async\n", func_ref->name);
        } else {
          fprintf(file, "@%s\n", func_ref->name);
        }
        FunctionRefMap_io_next(&func_iter);
      }
    }
    if (i < InstructionArray_size(&tape->ins)) {
      const Instruction *ins =
          InstructionArray_get_ref_unchecked(&tape->ins, i);
      int chars_written = instruction_write(ins, file, minimize);
      const SourceMapping *sm =
          SourceMappingArray_get_ref_unchecked(&tape->source_map, i);
      if (sm->col >= 0 && sm->line >= 0) {
        const int remaining_characters_before_comment =
            INSTRUCTION_COMMENT_LPAD - chars_written;
        int lpadding = minimize ? 0
                       : (remaining_characters_before_comment < 0)
                           ? 0
                           : remaining_characters_before_comment;
        fprintf(file, "%*s #%d %d", lpadding, PADDING, sm->line, sm->col);
      }
      fprintf(file, "\n");
    }
    if (in_class && ClassRefMap_io_has_next(&cls_iter)) {
      const ClassRef *class_ref = ClassRefMap_io_value(&cls_iter);
      if (class_ref->end_index == i || class_ref->end_index == i + 1) {
        in_class = false;
        if (minimize) {
          fprintf(file, "endclass\n");
        } else {
          fprintf(file, "endclass  ; %s\n", class_ref->name);
        }
        ClassRefMap_io_next(&cls_iter);
        // Handles classes with no body.
        if (ClassRefMap_io_has_next(&cls_iter) &&
            (ClassRefMap_io_value(&cls_iter))->start_index == i) {
          i--;
        }
      }
    }
  }
  const int num_src_lines = CharPtrArray_size(&tape->source_lines);
  if (num_src_lines > 0) {
    fprintf(file, "body\n");
    for (int i = 0; i < num_src_lines; ++i) {
      fprintf(file, "%s'%s'\n", minimize ? "" : " ",
              CharPtrArray_get_unchecked(&tape->source_lines, i));
    }
  }
}

// Consumes tail.
void tape_append(Tape *head, Tape *tail) {
  ASSERT(head != NULL);
  ASSERT(tail != NULL);

  int previous_head_length = InstructionArray_size(&head->ins);

  // Copy all instructions from tail to head.
  int i;
  for (i = 0; i < InstructionArray_size(&tail->ins); ++i) {
    Instruction *cpy = tape_add(head);
    SourceMapping *sm_cpy = tape_add_source(head, cpy);
    *cpy = InstructionArray_get_unchecked(&tail->ins, i);
    *sm_cpy = *tape_get_source(tail, i);
  }
  // Copy all functions.
  FunctionRefMapIOIterator func_iter;
  FunctionRefMap_io_iterator(&func_iter, &tail->func_refs);
  for (; FunctionRefMap_io_has_next(&func_iter);
       FunctionRefMap_io_next(&func_iter)) {
    FunctionRef *old_func = FunctionRefMap_io_mutable_value(&func_iter);
    FunctionRef *cpy;
    if (NULL != head->current_class) {
      FunctionRefMap_insert(&head->current_class->func_refs, old_func->name,
                            sizeof(char *), &cpy);
    } else {
      FunctionRefMap_insert(&head->func_refs, old_func->name, sizeof(char *),
                            &cpy);
    }
    *cpy = *old_func;
    cpy->index += previous_head_length;
  }
  // Copy all classes.
  ClassRefMapIOIterator class_iter;
  ClassRefMap_io_iterator(&class_iter, &tail->class_refs);
  for (; ClassRefMap_io_has_next(&class_iter);
       ClassRefMap_io_next(&class_iter)) {
    ClassRef *old_class = ClassRefMap_io_mutable_value(&class_iter);
    ClassRef *cpy_class;
    ClassRefMap_insert(&head->class_refs, old_class->name, sizeof(char *),
                       &cpy_class);
    *cpy_class = *old_class;
    cpy_class->start_index += previous_head_length;
    cpy_class->end_index += previous_head_length;
    // Copy all methods.
    FunctionRefMapIOIterator func_iter;
    FunctionRefMap_io_iterator(&func_iter, &old_class->func_refs);
    for (; FunctionRefMap_io_has_next(&func_iter);
         FunctionRefMap_io_next(&func_iter)) {
      FunctionRef *old_func = FunctionRefMap_io_mutable_value(&func_iter);
      FunctionRef *cpy;
      FunctionRefMap_insert(&cpy_class->func_refs, old_func->name,
                            sizeof(char *), &cpy);
      *cpy = *old_func;
      cpy->index += previous_head_length;
    }
    // Copy all fields.
    FieldRefMapIOIterator field_iter;
    FieldRefMap_io_iterator(&field_iter, &old_class->field_refs);
    for (; FieldRefMap_io_has_next(&field_iter);
         FieldRefMap_io_next(&field_iter)) {
      const FieldRef *old_field = FieldRefMap_io_mutable_value(&field_iter);
      FieldRef *cpy;
      FieldRefMap_insert(&cpy_class->field_refs, old_field->name,
                         sizeof(char *), &cpy);
      *cpy = *old_field;
    }
  }
  // Dealloc all of tail.
  InstructionArray_finalize(&tail->ins);
  SourceMappingArray_finalize(&tail->source_map);
  CharPtrArray_finalize(&tail->source_lines);
  ClassRefMap_finalize(&tail->class_refs);
  FunctionRefMap_finalize(&tail->func_refs);
  RELEASE(tail);
}

Token *q_peek_(TokenArray *tokens) {
  if (TokenArray_size(tokens) <= 0) {
    return NULL;
  }
  return TokenArray_get_unchecked(tokens, 0);
}

Token *next_token_skip_ln_(TokenArray *queue) {
  ASSERT(queue != NULL);
  ASSERT(TokenArray_size(queue) > 0);
  Token *first = TokenArray_remove_unchecked(queue, 0);
  ASSERT(first != NULL);
  while (first->type == TOKEN_NEWLINE) {
    first = q_peek_(queue);
    if (NULL == first) {
      return NULL;
    }
    TokenArray_remove_unchecked(queue, 0);
  }
  return first;
}

void tape_read_body_(Tape *const tape, TokenArray *tokens) {
  while (TokenArray_size(tokens) > 0) {
    Token *tok = next_token_skip_ln_(tokens);
    if (NULL == tok) {
      break;
    }
    ASSERT(token_type_is_string(tok->type));
    *CharPtrArray_push_back_ref(&tape->source_lines) = (char *)tok->text;
  }
}

bool is_body_token_(const Token *tok) {
  ASSERT(tok != NULL);
  if (NULL == tok) {
    return false;
  }
  return 0 == strcmp(BODY_KEYWORD, tok->text);
}

bool tape_read_ins(Tape *const tape, TokenArray *tokens) {
  ASSERT(tokens != NULL);
  if (TokenArray_size(tokens) < 1) {
    return false;
  }
  Token *first = next_token_skip_ln_(tokens);
  if (NULL == first || is_body_token_(first)) {
    return false;
  }
  if (SYMBOL_AT == first->type) {
    Token *fn_name = TokenArray_remove_unchecked(tokens, 0);
    if (SYMBOL_COLON == q_peek_(tokens)->type) {
      TokenArray_remove_unchecked(tokens, 0);
      Token *async_keyword = TokenArray_remove_unchecked(tokens, 0);
      if (0 != strcmp("async", async_keyword->text)) {
        FATALF("Invalid function qualifier '%s' on '%s'.", async_keyword->text,
               fn_name->text);
      }
      tape_label_async(tape, fn_name);
    } else {
      tape_label(tape, fn_name);
    }
    if (TOKEN_NEWLINE != q_peek_(tokens)->type) {
      FATALF("Invalid token after @def.");
    }
    return true;
  }
  if (0 == strcmp(CLASS_KEYWORD, first->text)) {
    Token *class_name = TokenArray_remove_unchecked(tokens, 0);
    if (TOKEN_NEWLINE == q_peek_(tokens)->type) {
      tape_class(tape, class_name);
      return true;
    }
    CharPtrArray parents;
    CharPtrArray_init(&parents);
    while (SYMBOL_COMMA == q_peek_(tokens)->type) {
      TokenArray_remove_unchecked(tokens, 0);
      *CharPtrArray_push_back_ref(&parents) =
          (char *)TokenArray_remove_unchecked(tokens, 0)->text;
    }
    tape_class_with_parents(tape, class_name, &parents);
    CharPtrArray_finalize(&parents);
    return true;
  }
  if (0 == strcmp(CLASSEND_KEYWORD, first->text)) {
    tape_endclass(tape, first);
    return true;
  }
  if (0 == strcmp(FIELD_KEYWORD, first->text)) {
    Token *field = TokenArray_remove_unchecked(tokens, 0);
    tape_field(tape, field->text);
    return true;
  }
  Op op = str_to_op(first->text);
  Token *next = (Token *)q_peek_(tokens);
  if (TOKEN_NEWLINE == next->type || SYMBOL_POUND == next->type) {
    tape_ins_no_arg(tape, op, first);
  } else if (SYMBOL_MINUS == next->type) {
    TokenArray_remove_unchecked(tokens, 0);
    tape_ins_neg(tape, op, TokenArray_remove_unchecked(tokens, 0));
  } else {
    TokenArray_remove_unchecked(tokens, 0);
    tape_ins(tape, op, next);
  }

  next = (Token *)q_peek_(tokens);
  if (next != NULL && SYMBOL_POUND == next->type) {
    TokenArray_remove_unchecked(tokens, 0);
    Primitive line = token_to_primitive(TokenArray_remove_unchecked(tokens, 0));
    Primitive col = token_to_primitive(TokenArray_remove_unchecked(tokens, 0));
    SourceMapping *sm =
        tape_add_source(tape, tape_get_mutable(tape, tape_size(tape) - 1));
    sm->source_line = pint(&line);
    sm->source_col = pint(&col);
    sm->source_token =
        token_create(sm->token->type, sm->source_line, sm->source_col,
                     sm->token->text, strlen(sm->token->text));
  }
  return true;
}

void tape_read(Tape *const tape, TokenArray *tokens) {
  ASSERT(tape != NULL);
  ASSERT(tokens != NULL);
  if (0 == strcmp(MODULE_KEYWORD, ((Token *)q_peek_(tokens))->text)) {
    TokenArray_remove_unchecked(tokens, 0);
    Token *module_name = TokenArray_remove_unchecked(tokens, 0);
    tape->module_name = module_name->text;
  } else {
    tape->module_name = global_intern("$");
  }
  if (TOKEN_NEWLINE == q_peek_(tokens)->type) {
    TokenArray_remove_unchecked(tokens, 0);
  }
  if (0 == strcmp(SOURCE_KEYWORD, ((Token *)q_peek_(tokens))->text)) {
    TokenArray_remove_unchecked(tokens, 0);
    Token *tok = TokenArray_remove_unchecked(tokens, 0);
    ASSERT(tok != NULL);
    tape_set_external_source(tape, tok->text);
  }
  while (TokenArray_size(tokens) > 0) {
    if (!tape_read_ins(tape, tokens)) {
      break;
    }
  }
  if (TokenArray_size(tokens) > 0) {
    tape_read_body_(tape, tokens);
  }
}

// **********************
// Specialized functions.
// **********************

int tape_ins_raw(Tape *tape, Instruction *ins) {
  ASSERT(tape != NULL);
  Instruction *new_ins = tape_add(tape);
  *new_ins = *ins;
  tape_add_source(tape, new_ins);
  return 1;
}

Primitive token_to_primitive(const Token *tok) {
  ASSERT(tok != NULL);
  Primitive val;
  switch (tok->type) {
    case TOKEN_INTEGER:
      val._type = PRIMITIVE_INT;
      val._int_val = (int64_t)strtoll(tok->text, NULL, 10);
      break;
    case TOKEN_FLOATING:
      val._type = PRIMITIVE_FLOAT;
      val._float_val = strtod(tok->text, NULL);
      break;
    default:
      FATALF("Attempted to create a Value from '%s'.", tok->text);
  }
  return val;
}

int tape_ins(Tape *tape, Op op, const Token *token) {
  ASSERT(tape != NULL);
  ASSERT(token != NULL);
  char *unescaped_str;
  Instruction *ins = tape_add(tape);
  SourceMapping *sm = tape_add_source(tape, ins);

  ins->op = op;
  sm->token = token;
  sm->line = token->line;
  sm->col = token->col;

  switch (token->type) {
    case TOKEN_INTEGER:
    case TOKEN_FLOATING:
      ins->type = INSTRUCTION_PRIMITIVE;
      ins->val = token_to_primitive(token);
      break;
    case STRING_IMMUTABLE:
    case STRING_SINGLEQUOTE:
      ins->type = INSTRUCTION_STRING;
      unescape(token->text, &unescaped_str);
      ins->str = global_intern(unescaped_str);
      RELEASE(unescaped_str);
      break;
    case TOKEN_WORD:
    default:
      ins->type = INSTRUCTION_ID;
      ins->id = token->text;
      break;
  }
  return 1;
}

int tape_ins_neg(Tape *tape, Op op, const Token *token) {
  ASSERT(tape != NULL);
  ASSERT(token != NULL);
  Instruction *ins = tape_add(tape);
  SourceMapping *sm = tape_add_source(tape, ins);

  ins->op = op;
  sm->token = token;
  sm->line = token->line;
  sm->col = token->col;

  ins->type = INSTRUCTION_PRIMITIVE;
  Primitive p = token_to_primitive(token);
  ins->val = primitive_int(-pint(&p));
  return 1;
}

int tape_ins_text(Tape *tape, Op op, const char text[], const Token *token) {
  ASSERT(tape != NULL);
  ASSERT(token != NULL);
  Instruction *ins = tape_add(tape);
  SourceMapping *sm = tape_add_source(tape, ins);

  ins->op = op;
  sm->token = token;
  sm->line = token->line;
  sm->col = token->col;

  ins->type = INSTRUCTION_ID;
  ins->id = text;
  return 1;
}

int tape_ins_int(Tape *tape, Op op, int val, const Token *token) {
  ASSERT(tape != NULL);
  ASSERT(token != NULL);
  Instruction *ins = tape_add(tape);
  SourceMapping *sm = tape_add_source(tape, ins);

  ins->op = op;
  sm->token = token;
  sm->line = token->line;
  sm->col = token->col;

  ins->type = INSTRUCTION_PRIMITIVE;
  ins->val._type = PRIMITIVE_INT;
  ins->val._int_val = val;
  return 1;
}

int tape_ins_no_arg(Tape *tape, Op op, const Token *token) {
  ASSERT(tape != NULL);
  ASSERT(token != NULL);
  Instruction *ins = tape_add(tape);
  SourceMapping *sm = tape_add_source(tape, ins);

  ins->op = op;
  sm->token = token;
  sm->line = token->line;
  sm->col = token->col;

  ins->type = INSTRUCTION_NO_ARG;
  return 1;
}

int tape_label(Tape *tape, const Token *token) {
  tape_start_func_(tape, token->text, /*is_async=*/false);
  return 0;
}

int tape_label_async(Tape *tape, const Token *token) {
  tape_start_func_(tape, token->text, /*is_async=*/true);
  return 0;
}

int tape_label_text(Tape *tape, const char text[]) {
  tape_start_func_(tape, text, /*is_async=*/false);
  return 0;
}

int tape_label_text_async(Tape *tape, const char text[]) {
  tape_start_func_(tape, text, /*is_async=*/true);
  return 0;
}

const char *anon_fn_for_token(const Token *token) {
  size_t needed = snprintf(NULL, 0, "$anon_%d_%d", token->line, token->col) + 1;
  char *buffer = MNEW_ARR(char, needed);
  snprintf(buffer, needed, "$anon_%d_%d", token->line, token->col);
  const char *label = global_intern(buffer);
  RELEASE(buffer);
  return label;
}

int tape_anon_label(Tape *tape, const Token *token) {
  const char *label = anon_fn_for_token(token);
  tape_label_text(tape, label);
  return 0;
}

int tape_anon_label_async(Tape *tape, const Token *token) {
  const char *label = anon_fn_for_token(token);
  tape_label_text_async(tape, label);
  return 0;
}

int tape_ins_anon(Tape *tape, Op op, const Token *token) {
  ASSERT(tape != NULL);
  ASSERT(token != NULL);
  Instruction *ins = tape_add(tape);
  SourceMapping *sm = tape_add_source(tape, ins);

  ins->op = op;
  sm->token = token;
  sm->line = token->line;
  sm->col = token->col;

  ins->type = INSTRUCTION_ID;
  ins->id = anon_fn_for_token(token);
  return 1;
}

int tape_module(Tape *tape, const Token *token) {
  tape->module_name = token->text;
  return 0;
}

int tape_class(Tape *tape, const Token *token) {
  tape_start_class(tape, token->text);
  return 0;
}

int tape_class_with_parents(Tape *tape, const Token *token,
                            CharPtrArray *q_parents) {
  tape_class(tape, token);
  ClassRef *cls = tape->current_class;
  while (CharPtrArray_size(q_parents) > 0) {
    char *parent = CharPtrArray_pop_back_unchecked(q_parents);
    CharPtrArray_push_back(&cls->supers, parent);
  }
  return 0;
}

int tape_endclass(Tape *tape, const Token *token) {
  tape_end_class(tape);
  return 0;
}

void tape_set_body(Tape *const tape, FileInfo *fi) {
  ASSERT(tape != NULL);
  ASSERT(fi != NULL);
  int len = file_info_len(fi);
  for (int i = 0; i < len; ++i) {
    char *line_escaped;
    escape(file_info_lookup(fi, i + 1)->line_text, &line_escaped);
    *CharPtrArray_push_back_ref(&tape->source_lines) =
        (char *)global_intern(line_escaped);
    RELEASE(line_escaped);
  }
}