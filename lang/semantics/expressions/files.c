// files.c
//
// Created on: Dec 29, 2019
//     Author: Jeff Manzione

#include "lang/semantics/expressions/files.h"

#include "alloc/arena/intern.h"
#include "debug/debug.h"
#include "lang/parser/parser.h"
#include "lang/semantics/expression_macros.h"
#include "lang/semantics/expression_tree.h"
#include "lang/semantics/expressions/classes.h"
#include "program/tape.h"
#include "struct/alist.h"
#include "vm/intern.h"

Argument populate_argument(const SyntaxTree *stree) {
  Argument arg = {.is_const = false,
                  .const_token = NULL,
                  .is_field = false,
                  .has_default = false,
                  .default_value = NULL};
  const SyntaxTree *argument = stree;
  if (IS_SYNTAX(argument, const_function_argument)) {
    arg.is_const = true;
    arg.const_token = argument->first->token;
    argument = argument->second;
  }
  if (IS_SYNTAX(argument, function_arg_elt_with_default)) {
    arg.has_default = true;
    arg.default_value = populate_expression(argument->second->second);
    argument = argument->first;
  }

  ASSERT(IS_SYNTAX(argument, identifier));
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

Arguments set_function_args(const SyntaxTree *stree, const Token *token) {
  Arguments args = {.token = token, .count_required = 0, .count_optional = 0};
  args.args = alist_create(Argument, 4);
  if (!IS_SYNTAX(stree, function_argument_list)) {
    Argument arg = populate_argument(stree);
    add_arg(&args, &arg);
    return args;
  }
  Argument arg = populate_argument(stree->first);
  add_arg(&args, &arg);
  const SyntaxTree *cur = stree->second;
  while (true) {
    if (IS_TOKEN(cur->first, COMMA)) {
      // Must be last arg.
      Argument arg = populate_argument(cur->second);
      add_arg(&args, &arg);
      break;
    }
    Argument arg = populate_argument(cur->first->second);
    add_arg(&args, &arg);
    cur = cur->second;
  }
  return args;
}

Annotation populate_annotation(const SyntaxTree *stree) {
  Annotation annot = {.prefix = NULL,
                      .class_name = NULL,
                      .is_called = false,
                      .has_args = false};
  if (!IS_SYNTAX(stree, annotation)) {
    ERROR("Must be annotation.");
  }
  if (IS_SYNTAX(stree->second, identifier)) {
    annot.class_name = stree->second->token;
  } else if (IS_SYNTAX(stree->second, annotation_not_called)) {
    annot.class_name = stree->second->second->second->token;
    annot.prefix = stree->second->first->token;
  } else if (IS_SYNTAX(stree->second, annotation_no_arguments)) {
    const SyntaxTree *class = stree->second->first;
    annot.is_called = true;
    annot.has_args = false;
    if (IS_SYNTAX(class, identifier)) {
      annot.class_name = class->token;
    } else if (IS_SYNTAX(class, annotation_not_called)) {
      annot.class_name = class->second->second->token;
      annot.prefix = class->first->token;
    }
  } else if (IS_SYNTAX(stree->second, annotation_with_arguments)) {
    const SyntaxTree *class = stree->second->first;
    const SyntaxTree *args_list = stree->second->second->second->first;
    const Token *lparen_tok = stree->second->second->first->token;
    annot.is_called = true;
    annot.has_args = true;
    if (IS_SYNTAX(class, identifier)) {
      annot.class_name = class->token;
    } else if (IS_SYNTAX(class, annotation_not_called)) {
      annot.class_name = class->second->second->token;
      annot.prefix = class->first->token;
    }
    annot.args = set_function_args(args_list, lparen_tok);
  }
  return annot;
}

void set_function_def(const SyntaxTree *fn_identifier, FunctionDef *func) {
  func->def_token = fn_identifier->first->token;
  func->fn_name = fn_identifier->second->token;
}

void _populate_function_qualifier(const SyntaxTree *fn_qualifier,
                                  bool *is_const, const Token **const_token,
                                  bool *is_async, const Token **async_token) {
  ASSERT(IS_SYNTAX(fn_qualifier, function_qualifier));
  if (IS_TOKEN(fn_qualifier, CONST_T)) {
    *is_const = true;
    *const_token = fn_qualifier->token;
  } else if (IS_TOKEN(fn_qualifier, ASYNC)) {
    *is_async = true;
    *async_token = fn_qualifier->token;
  } else {
    ERROR("Unknown function qualifier.");
  }
}

void populate_function_qualifiers(const SyntaxTree *fn_qualifiers,
                                  bool *is_const, const Token **const_token,
                                  bool *is_async, const Token **async_token) {
  if (IS_SYNTAX(fn_qualifiers, function_qualifier)) {
    _populate_function_qualifier(fn_qualifiers, is_const, const_token, is_async,
                                 async_token);
  } else if (IS_SYNTAX(fn_qualifiers, function_qualifier_list)) {
    ASSERT(IS_SYNTAX(fn_qualifiers->first, function_qualifier));
    ASSERT(IS_SYNTAX(fn_qualifiers->second, function_qualifier));
    _populate_function_qualifier(fn_qualifiers->first, is_const, const_token,
                                 is_async, async_token);
    _populate_function_qualifier(fn_qualifiers->second, is_const, const_token,
                                 is_async, async_token);
  } else {
    ERROR("unknown function qualifier list.");
  }
}

FunctionDef populate_function_variant(
    const SyntaxTree *stree, ParseExpression def,
    ParseExpression signature_with_qualifier,
    ParseExpression signature_no_qualifier, ParseExpression fn_identifier,
    ParseExpression function_arguments_no_args,
    ParseExpression function_arguments_present, FuncDefPopulator def_populator,
    FuncArgumentsPopulator args_populator) {
  FunctionDef func = {.def_token = NULL,
                      .fn_name = NULL,
                      .special_method = SpecialMethod__NONE,
                      .has_args = false,
                      .is_const = false,
                      .const_token = NULL,
                      .is_async = false,
                      .async_token = NULL,
                      .body = NULL};
  ASSERT(IS_SYNTAX(stree, def));

  const SyntaxTree *func_sig;
  if (IS_SYNTAX(stree->first, signature_with_qualifier)) {
    func_sig = stree->first->first;
    populate_function_qualifiers(stree->first->second, &func.is_const,
                                 &func.const_token, &func.is_async,
                                 &func.async_token);
  } else {
    ASSERT(IS_SYNTAX(stree->first, signature_no_qualifier));
    func_sig = stree->first;
  }

  ASSERT(IS_SYNTAX(func_sig->first, fn_identifier));
  def_populator(func_sig->first, &func);

  func.has_args = !IS_SYNTAX(func_sig->second, function_arguments_no_args);
  if (func.has_args) {
    ASSERT(IS_SYNTAX(func_sig->second, function_arguments_present));
    const SyntaxTree *func_args = func_sig->second->second->first;
    func.args = args_populator(func_args, func_sig->second->first->token);
  }
  func.body = populate_expression(stree->second);
  return func;
}

FunctionDef populate_function(const SyntaxTree *stree) {
  return populate_function_variant(
      stree, function_definition, function_signature_with_qualifier,
      function_signature_no_qualifier, def_identifier,
      function_arguments_no_args, function_arguments_present, set_function_def,
      set_function_args);
}

void delete_argument(Argument *arg) {
  if (arg->has_default) {
    delete_expression(arg->default_value);
  }
}

void _delete_argument_elt(void *ptr) {
  Argument *arg = (Argument *)ptr;
  delete_argument(arg);
}

void delete_arguments(Arguments *args) {
  alist_iterate(args->args, _delete_argument_elt);
  alist_delete(args->args);
}

void delete_function(FunctionDef *func) {
  if (func->has_args) {
    delete_arguments(&func->args);
  }
  delete_expression(func->body);
}

int produce_argument(Argument *arg, Tape *tape) {
  if (arg->is_field) {
    return tape_ins_no_arg(tape, PUSH, arg->arg_name) +
           tape_ins_text(tape, RES, SELF, arg->arg_name) +
           tape_ins(tape, arg->is_const ? FLDC : FLD, arg->arg_name);
  }
  return tape_ins(tape, arg->is_const ? LETC : LET, arg->arg_name);
}

int produce_all_arguments(Arguments *args, Tape *tape) {
  int i, num_ins = 0, num_args = alist_len(args->args);
  for (i = 0; i < num_args; ++i) {
    Argument *arg = (Argument *)alist_get(args->args, i);
    if (arg->has_default) {
      num_ins += tape_ins_no_arg(tape, PEEK, args->token) +
                 tape_ins_int(tape, TGTE, i + 1, arg->arg_name);

      Tape *tmp = tape_create();
      int default_ins = produce_instructions(arg->default_value, tmp);
      num_ins += tape_ins_int(tape, IFN, 3, arg->arg_name) +
                 tape_ins_no_arg(tape, (i == num_args - 1) ? RES : PEEK,
                                 arg->arg_name) +
                 tape_ins_int(tape, TGET, i, arg->arg_name) +
                 tape_ins_int(tape, JMP, default_ins, arg->arg_name) +
                 default_ins;
      tape_append(tape, tmp);
    } else {
      if (i == num_args - 1) {
        // Pop for last arg.
        num_ins += tape_ins_no_arg(tape, RES, args->token);
      } else {
        num_ins += tape_ins_no_arg(tape, PEEK, arg->arg_name);
      }
      num_ins += tape_ins_int(tape, TGET, i, args->token);
    }
    num_ins += produce_argument(arg, tape);
  }
  return num_ins;
}

int produce_arguments(Arguments *args, Tape *tape) {
  int num_args = alist_len(args->args);
  int i, num_ins = 0;
  if (num_args == 1) {
    Argument *arg = (Argument *)alist_get(args->args, 0);
    if (arg->has_default) {
      Tape *defaults = tape_create();
      int num_default_ins = produce_instructions(arg->default_value, defaults);
      num_ins += num_default_ins + tape_ins_no_arg(tape, PUSH, arg->arg_name) +
                 tape_ins_int(tape, TGTE, 1, arg->arg_name) +
                 tape_ins_int(tape, IF, num_default_ins + 1, arg->arg_name);
      tape_append(tape, defaults);
      num_ins += tape_ins_int(tape, JMP, 1, arg->arg_name) +
                 tape_ins_no_arg(tape, RES, arg->arg_name) +
                 tape_ins_int(tape, TGET, 0, arg->arg_name);
    }
    num_ins += produce_argument(arg, tape);
    return num_ins;
  }
  num_ins += tape_ins_no_arg(tape, PUSH, args->token);

  // Handle case where only 1 arg is passed and the rest are optional.
  Argument *first = (Argument *)alist_get(args->args, 0);
  num_ins += tape_ins_no_arg(tape, TLEN, first->arg_name) +
             tape_ins_no_arg(tape, PUSH, first->arg_name) +
             tape_ins_int(tape, PUSH, -1, first->arg_name) +
             tape_ins_no_arg(tape, EQ, first->arg_name);

  Tape *defaults = tape_create();
  int defaults_ins = 0;
  defaults_ins += tape_ins_no_arg(defaults, RES, first->arg_name) +
                  produce_argument(first, defaults);
  for (i = 1; i < num_args; ++i) {
    Argument *arg = (Argument *)alist_get(args->args, i);
    if (arg->has_default) {
      defaults_ins += produce_instructions(arg->default_value, defaults);
    } else {
      defaults_ins += tape_ins_no_arg(defaults, RNIL, arg->arg_name);
    }
    defaults_ins += produce_argument(arg, defaults);
  }

  Tape *non_defaults = tape_create();
  int nondefaults_ins = 0;
  nondefaults_ins += produce_all_arguments(args, non_defaults);

  defaults_ins += tape_ins_int(defaults, JMP, nondefaults_ins, first->arg_name);

  num_ins += tape_ins_int(tape, IFN, defaults_ins, first->arg_name);
  tape_append(tape, defaults);
  tape_append(tape, non_defaults);
  num_ins += defaults_ins + nondefaults_ins;
  return num_ins;
}

int produce_function_name(FunctionDef *func, Tape *tape) {
  // TODO: Handle async special functions.
  switch (func->special_method) {
  case SpecialMethod__NONE:
    return func->is_async ? tape_label_async(tape, func->fn_name)
                          : tape_label(tape, func->fn_name);
  case SpecialMethod__EQUIV:
    return tape_label_text(tape, EQ_FN_NAME);
  case SpecialMethod__NEQUIV:
    return tape_label_text(tape, NEQ_FN_NAME);
  case SpecialMethod__ARRAY_INDEX:
    return tape_label_text(tape, ARRAYLIKE_INDEX_KEY);
  case SpecialMethod__ARRAY_SET:
    return tape_label_text(tape, ARRAYLIKE_SET_KEY);
  default:
    ERROR("Unknown SpecialMethod.");
  }
  return 0;
}

int produce_function(FunctionDef *func, Tape *tape) {
  int num_ins = 0;
  num_ins += produce_function_name(func, tape);
  if (func->has_args) {
    num_ins += produce_arguments(&func->args, tape);
  }
  num_ins += produce_instructions(func->body, tape);
  // TODO: Uncomment when const is implemented.
  // if (func->is_const) {
  //   num_ins += tape_ins_no_arg(tape, CNST, func->const_token);
  // }
  num_ins += tape_ins_no_arg(tape, RET, func->def_token);
  return num_ins;
}

void populate_fi_statement(const SyntaxTree *stree, ModuleDef *module) {
  if (IS_SYNTAX(stree, module_statement)) {
    if (module->name.is_named) {
      ERROR("Module named twice: first '%s' then '%s'.",
            module->name.module_name->text, stree->second->token->text);
    }
    module->name.is_named = true;
    module->name.module_token = stree->first->token;
    module->name.module_name = stree->second->token;
  } else if (IS_SYNTAX(stree, import_statement)) {
    if (!IS_SYNTAX(stree->second, identifier)) {
      ERROR("import AS not yet supported.");
    }
    Import import = {.import_token = stree->first->token,
                     .module_name = stree->second->token};
    alist_append(module->imports, &import);
  } else if (IS_SYNTAX(stree, class_definition_no_annotation)) {
    ClassDef class = populate_class(stree);
    alist_append(module->classes, &class);
  } else if (IS_SYNTAX(stree, class_definition_with_annotation)) {
    ClassDef class = populate_class(stree->second);
    class.annot = populate_annotation(stree->first);
    alist_append(module->classes, &class);
  } else if (IS_SYNTAX(stree, function_definition)) {
    FunctionDef func = populate_function(stree);
    alist_append(module->functions, &func);
  } else {
    ExpressionTree *etree = populate_expression(stree);
    alist_append(module->statements, &etree);
  }
}

ModuleDef populate_module_def(const SyntaxTree *stree) {
  ModuleDef module_def;
  ModuleName module_name = {
      .is_named = false, .module_token = NULL, .module_name = NULL};
  module_def.name = module_name;
  module_def.imports = alist_create(Import, 4);
  module_def.classes = alist_create(ClassDef, 4);
  module_def.functions = alist_create(FunctionDef, 4);
  module_def.statements = alist_create(ExpressionTree *, DEFAULT_ARRAY_SZ);

  populate_fi_statement(stree->first, &module_def);
  const SyntaxTree *cur = stree->second;
  while (true) {
    if (!IS_SYNTAX(cur, file_level_statement_list1)) {
      populate_fi_statement(cur, &module_def);
      break;
    }
    populate_fi_statement(cur->first, &module_def);
    cur = cur->second;
  }
  return module_def;
}

ImplPopulate(file_level_statement_list, const SyntaxTree *stree) {
  file_level_statement_list->def = populate_module_def(stree);
}

void _delete_statement(void *ptr) {
  ExpressionTree *statement = *((ExpressionTree **)ptr);
  delete_expression(statement);
}

void delete_module_def(ModuleDef *module) {
  alist_delete(module->imports);

  alist_iterate(module->classes, (void (*)(void *))delete_class);
  alist_delete(module->classes);

  alist_iterate(module->functions, (void (*)(void *))delete_function);
  alist_delete(module->functions);

  alist_iterate(module->statements, _delete_statement);
  alist_delete(module->statements);
}

ImplDelete(file_level_statement_list) {
  delete_module_def(&file_level_statement_list->def);
}

int produce_module_def(ModuleDef *module, Tape *tape) {
  int num_ins = 0;
  if (module->name.is_named) {
    num_ins += tape_module(tape, module->name.module_name);
  }
  int i;
  // Imports
  for (i = 0; i < alist_len(module->imports); ++i) {
    Import *import = (Import *)alist_get(module->imports, i);
    num_ins += tape_ins(tape, LMDL, import->module_name);
  }
  // Superclasses
  for (i = 0; i < alist_len(module->classes); ++i) {
    ClassDef *class = (ClassDef *)alist_get(module->classes, i);
    if (alist_len(class->def.parent_classes) == 0) {
      continue;
    }
    ClassName *super = (ClassName *)alist_get(class->def.parent_classes, 0);
    num_ins += tape_ins(tape, PUSH, class->def.name.token) +
               tape_ins(tape, RES, super->token) +
               tape_ins_text(tape, CALL, intern("$__set_super"), super->token);
  }
  // Statements
  for (i = 0; i < alist_len(module->statements); ++i) {
    ExpressionTree *statement =
        *((ExpressionTree **)alist_get(module->statements, i));
    num_ins += produce_instructions(statement, tape);
  }
  num_ins += tape_ins_int(tape, EXIT, 0,
                          module->name.module_token == NULL
                              ? token_create(INTEGER, -1, -1, "0")
                              : module->name.module_token);
  for (i = 0; i < alist_len(module->classes); ++i) {
    ClassDef *class = (ClassDef *)alist_get(module->classes, i);
    num_ins += produce_class(class, tape);
  }
  for (i = 0; i < alist_len(module->functions); ++i) {
    FunctionDef *func = (FunctionDef *)alist_get(module->functions, i);
    num_ins += produce_function(func, tape);
  }
  return num_ins;
}

ImplProduce(file_level_statement_list, Tape *tape) {
  return produce_module_def(&file_level_statement_list->def, tape);
}
