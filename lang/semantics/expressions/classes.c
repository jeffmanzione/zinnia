// classes.c
//
// Created on: Dec 30, 2019
//     Author: Jeff Manzione

#include "lang/semantics/expressions/classes.h"

#include "alloc/arena/intern.h"
#include "lang/parser/parser.h"
#include "lang/semantics/expression_macros.h"
#include "lang/semantics/expression_tree.h"
#include "program/tape.h"
#include "struct/alist.h"
#include "struct/q.h"
#include "vm/intern.h"

void populate_class_def(ClassSignature *def, const SyntaxTree *stree) {
  def->parent_classes = alist_create(ClassName, 2);
  // No parent classes.
  if (IS_SYNTAX(stree, identifier)) {
    def->name.token = stree->token;
  } else if (IS_SYNTAX(stree, class_name_and_inheritance)) {
    const SyntaxTree *class_inheritance = stree;
    ASSERT(IS_SYNTAX(class_inheritance->first, identifier));
    def->name.token = class_inheritance->first->token;
    ASSERT(IS_SYNTAX(class_inheritance->second, parent_classes));
    if (IS_SYNTAX(class_inheritance->second->second, identifier)) {
      ClassName name = {.token = class_inheritance->second->second->token};
      alist_append(def->parent_classes, &name);
    } else {
      ASSERT(IS_SYNTAX(class_inheritance->second->second, parent_class_list));
      ClassName name = {.token =
                            class_inheritance->second->second->first->token};
      alist_append(def->parent_classes, &name);
      const SyntaxTree *parent_class =
          class_inheritance->second->second->second;
      while (true) {
        if (IS_SYNTAX(parent_class, parent_class_list1)) {
          if (IS_TOKEN(parent_class->first, COMMA)) {
            name.token = parent_class->second->token;
            alist_append(def->parent_classes, &name);
            break;
          } else {
            name.token = parent_class->first->second->token;
            alist_append(def->parent_classes, &name);
            parent_class = parent_class->second;
          }
        } else {
          ASSERT(IS_SYNTAX(parent_class, identifier));
          name.token = parent_class->token;
          alist_append(def->parent_classes, &name);
          break;
        }
      }
    }
  } else {
    ERROR("Unknown class name composition.");
  }
}

Argument populate_constructor_argument(const SyntaxTree *stree) {
  Argument arg = {.is_const = false,
                  .const_token = NULL,
                  .is_field = false,
                  .has_default = false,
                  .default_value = NULL};
  const SyntaxTree *argument = stree;
  if (IS_SYNTAX(argument, const_new_argument)) {
    arg.is_const = true;
    arg.const_token = argument->first->token;
    argument = argument->second;
  }
  if (IS_SYNTAX(argument, new_arg_elt_with_default)) {
    arg.has_default = true;
    arg.default_value = populate_expression(argument->second->second);
    argument = argument->first;
  }
  if (IS_SYNTAX(argument, new_field_arg)) {
    arg.arg_name = argument->second->token;
    arg.is_field = true;
  } else {
    ASSERT(IS_SYNTAX(argument, identifier));
    arg.arg_name = argument->token;
  }
  return arg;
}

Arguments set_constructor_args(const SyntaxTree *stree, const Token *token) {
  Arguments args = {.token = token, .count_required = 0, .count_optional = 0};
  args.args = alist_create(Argument, 4);
  if (!IS_SYNTAX(stree, new_argument_list)) {
    Argument arg = populate_constructor_argument(stree);
    add_arg(&args, &arg);
    return args;
  }
  Argument arg = populate_constructor_argument(stree->first);
  add_arg(&args, &arg);
  const SyntaxTree *cur = stree->second;
  while (true) {
    if (IS_TOKEN(cur->first, COMMA)) {
      // Must be last arg.
      Argument arg = populate_constructor_argument(cur->second);
      add_arg(&args, &arg);
      break;
    }
    Argument arg = populate_constructor_argument(cur->first->second);
    add_arg(&args, &arg);
    cur = cur->second;
  }
  return args;
}

void set_method_def(const SyntaxTree *fn_identifier, FunctionDef *func) {
  ASSERT(IS_SYNTAX(fn_identifier, method_identifier));
  func->def_token = fn_identifier->first->token;
  const SyntaxTree *fn_name = fn_identifier->second;
  if (IS_SYNTAX(fn_name, identifier)) {
    func->fn_name = fn_name->token;
    func->special_method = SpecialMethod__NONE;
  } else if (IS_TOKEN(fn_name, EQUIV)) {
    func->fn_name = fn_name->token;
    func->special_method = SpecialMethod__EQUIV;
  } else if (IS_TOKEN(fn_name, NEQUIV)) {
    func->fn_name = fn_name->token;
    func->special_method = SpecialMethod__NEQUIV;
  } else if (IS_TOKEN2(fn_name, LBRAC, RBRAC)) {
    func->fn_name = fn_name->first->token;
    func->special_method = SpecialMethod__ARRAY_INDEX;
  } else if (IS_TOKEN3(fn_name, LBRAC, RBRAC, EQUALS)) {
    func->fn_name = fn_name->first->token;
    func->special_method = SpecialMethod__ARRAY_SET;
  } else {
    ERROR("Unknown method name type.");
  }
}

FunctionDef populate_method(const SyntaxTree *stree) {
  return populate_function_variant(
      stree, method_definition, method_signature_with_qualifier,
      method_signature_no_qualifier, method_identifier,
      function_arguments_no_args, function_arguments_present, set_method_def,
      set_function_args);
}

void set_new_def(const SyntaxTree *fn_identifier, FunctionDef *func) {
  ASSERT(IS_SYNTAX(fn_identifier, new_expression));
  func->def_token = fn_identifier->token;
  func->fn_name = fn_identifier->token;
}

FunctionDef populate_constructor(const SyntaxTree *stree) {
  return populate_function_variant(stree, new_definition, new_signature_const,
                                   new_signature_nonconst, new_expression,
                                   new_arguments_no_args, new_arguments_present,
                                   set_new_def, set_constructor_args);
}

FieldDef populate_field_statement(const Token *field_token,
                                  const SyntaxTree *stree) {
  FieldDef field = {.name = stree->token, .field_token = field_token};
  return field;
}

void populate_field_statements(const SyntaxTree *stree, ClassDef *class) {
  ASSERT(IS_TOKEN(stree->first, FIELD));
  const Token *field_token = stree->first->token;

  if (!IS_SYNTAX(stree->second, identifier_list)) {
    ASSERT(IS_SYNTAX(stree->second, identifier));
    FieldDef field = populate_field_statement(field_token, stree->second);
    alist_append(class->fields, &field);
    return;
  }
  FieldDef field = populate_field_statement(field_token, stree->second->first);
  alist_append(class->fields, &field);

  const SyntaxTree *statement = stree->second->second;
  while (true) {
    if (IS_TOKEN(statement->first, COMMA)) {
      FieldDef field = populate_field_statement(field_token, statement->second);
      alist_append(class->fields, &field);
      break;
    }
    FieldDef field =
        populate_field_statement(field_token, statement->first->second);
    alist_append(class->fields, &field);
    statement = statement->second;
  }
}

void populate_class_statement(ClassDef *class, const SyntaxTree *stree) {
  if (IS_SYNTAX(stree, field_statement)) {
    populate_field_statements(stree, class);
  } else if (IS_SYNTAX(stree, method_definition)) {
    FunctionDef method = populate_method(stree);
    alist_append(class->methods, &method);
  } else if (IS_SYNTAX(stree, new_definition)) {
    class->constructor = populate_constructor(stree);
    class->has_constructor = true;
  } else {
    ERROR("Unknown class_statement.");
  }
}

void populate_class_statements(ClassDef *class, const SyntaxTree *stree) {
  ASSERT(IS_TOKEN(stree->first, LBRCE));
  if (IS_TOKEN(stree->second, RBRCE)) {
    // Empty body.
    return;
  }
  if (!IS_SYNTAX(stree->second->first, class_statement_list)) {
    populate_class_statement(class, stree->second->first);
    return;
  }
  populate_class_statement(class, stree->second->first->first);
  const SyntaxTree *statement = stree->second->first->second;
  while (true) {
    if (!IS_SYNTAX(statement, class_statement_list1)) {
      populate_class_statement(class, statement);
      break;
    }
    populate_class_statement(class, statement->first);
    statement = statement->second;
  }
}

ClassDef populate_class(const SyntaxTree *stree) {
  ASSERT(!IS_LEAF(stree->first), IS_TOKEN(stree->first->first, CLASS));
  ClassDef class;
  class.has_constructor = false;
  class.fields = alist_create(FieldDef, 4);
  class.methods = alist_create(FunctionDef, 6);
  populate_class_def(&class.def, stree->first->second);

  const SyntaxTree *body = stree->second;
  if (IS_SYNTAX(body, class_compound_statement)) {
    populate_class_statements(&class, body);
  } else {
    populate_class_statement(&class, body);
  }
  return class;
}

void delete_class(ClassDef *class) {
  if (class->has_constructor) {
    delete_function(&class->constructor);
  }
  alist_delete(class->def.parent_classes);
  alist_delete(class->fields);
  int i;
  for (i = 0; i < alist_len(class->methods); ++i) {
    FunctionDef *func = (FunctionDef *)alist_get(class->methods, i);
    delete_function(func);
  }
  alist_delete(class->methods);
}

int produce_constructor(ClassDef *class, Tape *tape) {
  int num_ins = 0;

  if (class->has_constructor) {
    num_ins += tape_label(tape, class->constructor.fn_name);
  } else {
    num_ins += tape_label_text(tape, CONSTRUCTOR_KEY);
  }

  int num_fields = alist_len(class->fields);
  if (num_fields > 0) {
    num_ins += tape_ins_no_arg(tape, PUSH, class->def.name.token);
    int i;
    for (i = 0; i < num_fields; ++i) {
      FieldDef *field = (FieldDef *)alist_get(class->fields, i);
      num_ins += tape_ins_no_arg(tape, PNIL, field->name) +
                 tape_ins_text(tape, RES, SELF, field->name) +
                 tape_ins(tape, FLD, field->name);
    }
    num_ins += tape_ins_no_arg(tape, RES, class->def.name.token);
  }

  if (class->has_constructor && class->constructor.has_args) {
    num_ins += produce_arguments(&class->constructor.args, tape);
  }

  if (class->has_constructor) {
    FunctionDef *func = &class->constructor;
    num_ins += produce_instructions(func->body, tape);
    num_ins += tape_ins_text(tape, RES, SELF, func->fn_name);
    // TODO: Uncomment when const is implemented.
    // if (func->is_const) {
    //   num_ins += tape_ins_no_arg(tape, CNST, func->const_token);
    // }
    num_ins += tape_ins_no_arg(tape, RET, func->def_token);
  } else {
    num_ins += tape_ins_text(tape, RES, SELF, class->def.name.token);
    num_ins += tape_ins_no_arg(tape, RET, class->def.name.token);
  }
  return num_ins;
}

int produce_class(ClassDef *class, Tape *tape) {
  int num_ins = 0;
  if (alist_len(class->def.parent_classes) == 0) {
    // No parents.
    num_ins += tape_class(tape, class->def.name.token);
  } else {
    Q parents;
    Q_init(&parents);
    int i;
    for (i = 0; i < alist_len(class->def.parent_classes); ++i) {
      ClassName *name = (ClassName *)alist_get(class->def.parent_classes, i);
      Q_enqueue(&parents, (char *)name->token->text);
    }
    num_ins += tape_class_with_parents(tape, class->def.name.token, &parents);
    Q_finalize(&parents);
  }
  // Fields
  AL_iter fields = alist_iter(class->fields);
  for (; al_has(&fields); al_inc(&fields)) {
    FieldDef *f = (FieldDef *)al_value(&fields);
    tape_field(tape, f->name->text);
  }
  // Constructor
  if (class->has_constructor || alist_len(class->fields) > 0) {
    num_ins += produce_constructor(class, tape);
  }
  int i, num_methods = alist_len(class->methods);
  for (i = 0; i < num_methods; ++i) {
    FunctionDef *func = (FunctionDef *)alist_get(class->methods, i);
    num_ins += produce_function(func, tape);
  }
  num_ins += tape_endclass(tape, class->def.name.token);
  return num_ins;
}
